/**
 * @file
 * VuoCompositionLoader implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

extern "C" {

#include <dlfcn.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <pthread.h>
#include "VuoTelemetry.hh"
#include "VuoEventLoop.h"
#include "VuoRuntime.h"
#include "VuoCompositionState.h"

void *ZMQLoaderControlContext = NULL;  ///< The context for initializing sockets to control the composition loader.
void *ZMQLoaderControl = NULL;  ///< The socket for controlling the composition loader.
static void *ZMQLoaderSelfReceive = 0;        ///< Used to break out of a ZMQLoaderControl poll.
static void *ZMQLoaderSelfSend = 0;           ///< Used to break out of a ZMQLoaderControl poll.

void *ZMQControlContext = NULL;  ///< The context for initializing sockets to control the composition.
void *ZMQControl = NULL;  ///< The socket for controlling the composition.
char *controlURL = NULL;  ///< The URL that the composition will use to initialize its control socket.
char *telemetryURL = NULL;  ///< The URL that the composition will use to initialize its telemetry socket.

bool isReplacing = false;  ///< True if the composition is in the process of being replaced.
void *dylibHandle = NULL;  ///< A handle to the running composition.
map<string, void *> resourceDylibHandles;  ///< Handles to the running composition's resources.
vector<string> resourceDylibsToUnload;  ///< Paths of the resources to be unloaded when the composition is replaced.
vector<string> resourceDylibsToLoad;  ///< Paths of the resources to be loaded (or reloaded) when the composition is replaced.
pid_t runnerPid = 0;  ///< Process ID of the runner that started the composition.
int runnerPipe = -1;  ///< The file descriptor for the composition's end of the pipe used to detect if the runner's process ends.
bool continueIfRunnerDies = false;  ///< If true, the composition continues running if the runner's process ends.

bool replaceComposition(const char *dylibPath, char *compositionDiff);
void stopComposition(void);
bool loadResourceDylib(const string &dylibPath);
bool unloadResourceDylib(const string &dylibPath);

void *VuoApp_mainThread = NULL;	///< A reference to the main thread
char *VuoApp_dylibPath = NULL;	///< The path of the most recently loaded composition dylib.

/**
 * Placeholder until `vuoIsCurrentCompositionStopped` becomes available.
 */
static bool isStoppedInitially(void)
{
	return false;
}

VuoIsCurrentCompositionStoppedType *isStopped = isStoppedInitially;  ///< Reference to `vuoIsCurrentCompositionStopped()` once it becomes available.

} // extern "C"

/**
 * Get a reference to the main thread, so we can perform runtime thread assertions.
 */
static void __attribute__((constructor)) VuoCompositionLoader_init(void)
{
	VuoApp_mainThread = (void *)pthread_self();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// Calls _TSGetMainThread().
	// https://b33p.net/kosada/node/12944
	YieldToAnyThread();
#pragma clang diagnostic pop
}

/**
 * Sends a control reply message to the process controlling this composition loader.
 */
void vuoLoaderControlReplySend(enum VuoLoaderControlReply reply, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoLoaderControl",ZMQLoaderControl,reply,messages,messageCount,false,NULL);
}

/**
 * Sends a control request message to the running composition.
 */
void vuoControlRequestSend(enum VuoControlRequest request, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoControl",ZMQControl,request,messages,messageCount,false,NULL);
}

/**
 * Receives a control reply message from the running composition, and checks that it is the expected reply.
 */
void vuoControlReplyReceive(enum VuoControlReply expectedReply)
{
	int reply = vuoReceiveInt(ZMQControl, NULL);
	if (reply != expectedReply)
		VUserLog("The composition loader received the wrong message from the composition (expected %d, received %d)", expectedReply, reply);
}

/**
 * Starts the composition loader.
 */
int main(int argc, char **argv)
{
	char *loaderControlURL = NULL;

	// Parse commandline arguments.
	{
		static struct option options[] = {
			{"vuo-control", required_argument, NULL, 0},
			{"vuo-telemetry", required_argument, NULL, 0},
			{"vuo-loader", required_argument, NULL, 0},
			{"vuo-runner-pipe", required_argument, NULL, 0},
			{"vuo-continue-if-runner-dies", no_argument, NULL, 0},
			{"vuo-runner-pid", required_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		int ret;
		while ((ret = getopt_long(argc, argv, "", options, &optionIndex)) != -1)
		{
			if (ret == '?')
				continue;

			switch(optionIndex)
			{
				case 0:	// "vuo-control"
					controlURL = strdup(optarg);
					break;
				case 1:	// "vuo-telemetry"
					telemetryURL = strdup(optarg);
					break;
				case 2:	// "vuo-loader"
					if (loaderControlURL)
						free(loaderControlURL);
					loaderControlURL = strdup(optarg);
					break;
				case 3:  // --vuo-runner-pipe
					runnerPipe = atoi(optarg);
					break;
				case 4:  // --vuo-continue-if-runner-dies
					continueIfRunnerDies = true;
					break;
				case 5:  // --vuo-runner-pid
					runnerPid = atoi(optarg);
					break;
			}
		}
	}

	if (!loaderControlURL)
	{
		VUserLog("Error: Please specify a --vuo-loader URL.");
		return -1;
	}

	VuoDefer(^{ free(loaderControlURL); });

	// Set up ZMQ connections.
	{
		ZMQLoaderControlContext = zmq_init(1);

		ZMQLoaderControl = zmq_socket(ZMQLoaderControlContext,ZMQ_REP);
		if(zmq_bind(ZMQLoaderControl,loaderControlURL))
		{
			VUserLog("The composition couldn't start because the composition loader couldn't establish communication to control the composition : %s", zmq_strerror(errno));
			return -1;
		}

		ZMQLoaderSelfReceive = zmq_socket(ZMQLoaderControlContext, ZMQ_PAIR);
		if (zmq_bind(ZMQLoaderSelfReceive, "inproc://vuo-loader-self") != 0)
		{
			VUserLog("Couldn't bind self-receive socket: %s (%d)", zmq_strerror(errno), errno);
			return -1;
		}

		ZMQLoaderSelfSend = zmq_socket(ZMQLoaderControlContext, ZMQ_PAIR);
		if (zmq_connect(ZMQLoaderSelfSend, "inproc://vuo-loader-self") != 0)
		{
			VUserLog("Couldn't connect self-send socket: %s (%d)", zmq_strerror(errno), errno);
			return -1;
		}
	}

	// Launch control responder.
	dispatch_queue_t loaderControlQueue;
	dispatch_source_t loaderControlTimer;
	dispatch_semaphore_t loaderControlCanceledSemaphore;
	{
		loaderControlCanceledSemaphore = dispatch_semaphore_create(0);
		loaderControlQueue = dispatch_queue_create("org.vuo.runtime.loader", NULL);
		loaderControlTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, loaderControlQueue);
		dispatch_source_set_timer(loaderControlTimer, dispatch_walltime(NULL,0), NSEC_PER_SEC/1000, NSEC_PER_SEC/1000);
		dispatch_source_set_event_handler(loaderControlTimer, ^{

											  vuoMemoryBarrier();

											  zmq_pollitem_t items[]=
											  {
												  {ZMQLoaderControl,0,ZMQ_POLLIN,0},
												  {ZMQLoaderSelfReceive,0,ZMQ_POLLIN,0},
											  };
											  int itemCount = 2;
											  long timeout = -1;  // Wait forever (we'll get a message on ZMQLoaderSelfReceive when it's time to stop).
											  zmq_poll(items,itemCount,timeout);
											  if(!(items[0].revents & ZMQ_POLLIN))
												  return;

											  enum VuoLoaderControlRequest control = (enum VuoLoaderControlRequest) vuoReceiveInt(ZMQLoaderControl, NULL);

											  switch (control)
											  {
												  case VuoLoaderControlRequestCompositionReplace:
												  {
													  char *dylibPath = vuoReceiveAndCopyString(ZMQLoaderControl, NULL);

													  int numResourceDylibPathsAdded = vuoReceiveInt(ZMQLoaderControl, NULL);
													  for (int i = 0; i < numResourceDylibPathsAdded; ++i)
													  {
														  char *s = vuoReceiveAndCopyString(ZMQLoaderControl, NULL);
														  resourceDylibsToLoad.push_back(s);
														  free(s);
													  }

													  int numResourceDylibPathsRemoved = vuoReceiveInt(ZMQLoaderControl, NULL);
													  for (int i = 0; i < numResourceDylibPathsRemoved; ++i)
													  {
														  char *s = vuoReceiveAndCopyString(ZMQLoaderControl, NULL);
														  resourceDylibsToUnload.push_back(s);
														  free(s);
													  }

													  char *compositionDiff = vuoReceiveAndCopyString(ZMQLoaderControl, NULL);

													  bool ok = replaceComposition(dylibPath, compositionDiff);

													  vuoLoaderControlReplySend(VuoLoaderControlReplyCompositionReplaced,NULL,0);

													  if (! ok)
													  {
														  exit(-1);
													  }
													  break;
												  }
											  }
										  });
		dispatch_source_set_cancel_handler(loaderControlTimer, ^{
											   dispatch_semaphore_signal(loaderControlCanceledSemaphore);
										   });
		dispatch_resume(loaderControlTimer);
	}

	// Wait until the composition is permanently stopped (not just temporarily stopped for replacing).
	{
		// Before the composition has started for the first time, isStopped is an alias for isStoppedInitially, which returns false.
		// While the composition is being replaced, isReplaced is true and isStopped is invalid for part of the time.
		// While the composition is running (started and not yet stopped), isStopped returns false.
		while (isReplacing || ! isStopped())
			VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
	}

	// Clean up ZMQ connections.
	{
		vuoMemoryBarrier();

		zmq_close(ZMQControl);

		dispatch_source_cancel(loaderControlTimer);

		// Break out of zmq_poll().
		{
			char z = 0;
			zmq_msg_t message;
			zmq_msg_init_size(&message, sizeof z);
			memcpy(zmq_msg_data(&message), &z, sizeof z);
			if (zmq_msg_send(&message, static_cast<zmq_msg_t *>(ZMQLoaderSelfSend), 0) == -1)
				VUserLog("Couldn't break: %s (%d)", zmq_strerror(errno), errno);
			zmq_msg_close(&message);
		}

		dispatch_semaphore_wait(loaderControlCanceledSemaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(loaderControlCanceledSemaphore);
		dispatch_release(loaderControlTimer);
		dispatch_sync(loaderControlQueue, ^{
			zmq_close(ZMQLoaderControl);
			zmq_close(ZMQLoaderSelfSend);
			zmq_close(ZMQLoaderSelfReceive);
		});
		dispatch_release(loaderControlQueue);

		VuoFiniType *vuoFini = (VuoFiniType *) dlsym(dylibHandle, "vuoFini");
		if (! vuoFini)
		{
			VUserLog("The composition couldn't stop because vuoFini() couldn't be found in the composition library : %s", dlerror());
			return -1;
		}
		vuoFini();
	}

	return 0;
}

/**
 * Replaces the currently running composition (if any) with the given composition.
 *
 * The runtime takes ownership of @a compositionDiff.
 */
bool replaceComposition(const char *dylibPath, char *compositionDiff)
{
	// Store dylibPath for VuoApp_getName().
	if (VuoApp_dylibPath)
		free(VuoApp_dylibPath);
	VuoApp_dylibPath = strdup(dylibPath);
	// Provide the composition name to the crash reporter.
	{
		const char *filename = strrchr(dylibPath, '/');
		if (filename)
		{
			char *name = strdup(filename + 1); // Trim leading slash.
			name[strlen(name) - strlen("-XXXXXX.dylib")] = 0;
			VuoLog_status("Running Vuo composition \"%s\"", name);
			free(name);
		}
	}

	isReplacing = true;

	void *runtimePersistentState = NULL;

	// Stop the old composition (if any).
	if (dylibHandle)
	{
		isStopped = NULL;

		VuoCompositionState compositionState = { NULL, "" };
		void **vuoRuntimeState = (void **)dlsym(dylibHandle, "vuoRuntimeState");
		if (! vuoRuntimeState)
		{
			VUserLog("The composition couldn't be replaced because vuoRuntimeState couldn't be found in '%s' : %s", dylibPath, dlerror());
			return false;
		}
		compositionState.runtimeState = *vuoRuntimeState;

		typedef void (*vuoSetCompositionDiffType)(VuoCompositionState *, char *);
		vuoSetCompositionDiffType vuoSetCompositionDiff = (vuoSetCompositionDiffType) dlsym(dylibHandle, "vuoSetCompositionDiff");
		if (! vuoSetCompositionDiff)
		{
			VUserLog("The composition couldn't be replaced because vuoSetCompositionDiff() couldn't be found in the composition library : %s", dlerror());
			return false;
		}
		vuoSetCompositionDiff(&compositionState, compositionDiff);

		stopComposition();

		zmq_close(ZMQControl);
		ZMQControl = NULL;

		VuoFiniType *vuoFini = (VuoFiniType *)dlsym(dylibHandle, "vuoFini");
		if (! vuoFini)
		{
			VUserLog("The composition couldn't be replaced because vuoFini() couldn't be found in the composition library : %s", dlerror());
			return false;
		}
		runtimePersistentState = vuoFini();

		dlclose(dylibHandle);
		dylibHandle = NULL;

		for (vector<string>::iterator i = resourceDylibsToUnload.begin(); i != resourceDylibsToUnload.end(); ++i)
			unloadResourceDylib(*i);
		resourceDylibsToUnload.clear();
	}

	// Start the new composition paused.
	{
		for (vector<string>::iterator i = resourceDylibsToLoad.begin(); i != resourceDylibsToLoad.end(); ++i)
			loadResourceDylib(*i);
		resourceDylibsToLoad.clear();

		ZMQControlContext = zmq_init(1);

		ZMQControl = zmq_socket(ZMQControlContext,ZMQ_REQ);
		if (zmq_connect(ZMQControl,controlURL))
		{
			VUserLog("The composition couldn't be replaced because the composition loader couldn't establish communication to control the composition : %s", zmq_strerror(errno));
			return false;
		}

		VuoInitInProcessType *vuoInitInProcess = NULL;

		dylibHandle = dlopen(dylibPath, RTLD_NOW);
		if (! dylibHandle)
		{
			VUserLog("The composition couldn't be replaced because the library '%s' couldn't be loaded : %s", dylibPath, dlerror());
			return false;
		}

		vuoInitInProcess = (VuoInitInProcessType *)dlsym(dylibHandle, "vuoInitInProcess");
		if (! vuoInitInProcess)
		{
			VUserLog("The composition couldn't be replaced because vuoInitInProcess() couldn't be found in '%s' : %s", dylibPath, dlerror());
			return false;
		}

		isStopped = (VuoIsCurrentCompositionStoppedType *)dlsym(dylibHandle, "vuoIsCurrentCompositionStopped");
		if (! isStopped)
		{
			VUserLog("The composition couldn't be replaced because vuoIsCurrentCompositionStopped() couldn't be found in '%s' : %s", dylibPath, dlerror());
			return false;
		}

		vuoInitInProcess(ZMQControlContext, controlURL, telemetryURL, true, runnerPid, runnerPipe, continueIfRunnerDies,
						 "", dylibHandle, runtimePersistentState, false);
	}

	isReplacing = false;

	return true;
}

/**
 * Sends a control request to the composition telling it to stop.
 */
void stopComposition(void)
{
	vuoMemoryBarrier();

	const int timeoutInSeconds = -1;
	zmq_msg_t messages[2];
	vuoInitMessageWithInt(&messages[0], timeoutInSeconds);
	vuoInitMessageWithBool(&messages[1], true ); // isBeingReplaced
	vuoControlRequestSend(VuoControlRequestCompositionStop, messages, 2);
	vuoControlReplyReceive(VuoControlReplyCompositionStopping);
}

/**
 * Loads the resource dylib at the given path and adds the resulting handle to the list of resource dylibs.
 */
bool loadResourceDylib(const string &dylibPath)
{
	void *dylibHandle = dlopen(dylibPath.c_str(), RTLD_NOW);
	if (! dylibHandle)
	{
		VUserLog("The composition couldn't be replaced because the library '%s' couldn't be loaded : %s", dylibPath.c_str(), dlerror());
		return false;
	}

	resourceDylibHandles[dylibPath] = dylibHandle;
	return true;
}

/**
 * Unloads the resource dylib at the given path and removes its handle from the list of resource dylibs.
 */
bool unloadResourceDylib(const string &dylibPath)
{
	void *dylibHandle = resourceDylibHandles[dylibPath];
	if (! dylibHandle)
	{
		VUserLog("The library '%s' couldn't be unloaded because its handle was not found.", dylibPath.c_str());
		return false;
	}

	int ret = dlclose(dylibHandle);
	if (ret != 0)
	{
		VUserLog("The library '%s' couldn't be unloaded : %s", dylibPath.c_str(), dlerror());
		return false;
	}

	resourceDylibHandles.erase(dylibPath);
	return true;
}
