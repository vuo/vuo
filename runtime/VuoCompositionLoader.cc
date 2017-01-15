/**
 * @file
 * VuoCompositionLoader implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C" {

#include <dlfcn.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <pthread.h>
#include "VuoTelemetry.h"
#include "VuoEventLoop.h"
#include "VuoRuntime.h"

void *ZMQLoaderControlContext = NULL;  ///< The context for initializing sockets to control the composition loader.
void *ZMQLoaderControl = NULL;  ///< The socket for controlling the composition loader.
void *ZMQControlContext = NULL;  ///< The context for initializing sockets to control the composition.
void *ZMQControl = NULL;  ///< The socket for controlling the composition.
char *controlURL = NULL;  ///< The URL that the composition will use to initialize its control socket.
char *telemetryURL = NULL;  ///< The URL that the composition will use to initialize its telemetry socket.
bool *isStopped = NULL;  ///< True if the composition has stopped.
bool isReplacing = false;  ///< True if the composition is in the process of being replaced.
void *dylibHandle = NULL;  ///< A handle to the running composition.
void **resourceDylibHandles = NULL;  ///< A list of handles to the running composition's resources.
size_t resourceDylibHandlesSize = 0;  ///< The number of items in @c resourceDylibHandles.
size_t resourceDylibHandlesCapacity = 0;  ///< The number of items that @c resourceDylibHandlesCapacity can currently hold.
pid_t runnerPid = 0;  ///< Process ID of the runner that started the composition.
int runnerPipe = -1;  ///< The file descriptor for the composition's end of the pipe used to detect if the runner's process ends.
bool continueIfRunnerDies = false;  ///< If true, the composition continues running if the runner's process ends.
bool trialRestrictionsEnabled = true;	///< If true, some nodes may restrict how they can be used.

bool replaceComposition(const char *dylibPath, char *compositionDiff);
void stopComposition(void);
bool loadResourceDylib(const char *resourceDylibPath);
void unloadResourceDylibs(void);

void *VuoApp_mainThread = NULL;	///< A reference to the main thread

} // extern "C"

/**
 * Get a reference to the main thread, so we can perform runtime thread-sanity assertions.
 */
static void __attribute__((constructor)) VuoCompositionLoader_init(void)
{
	VuoApp_mainThread = (void *)pthread_self();
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
	runnerPid = getppid();

	// Parse commandline arguments.
	{
		static struct option options[] = {
			{"vuo-control", required_argument, NULL, 0},
			{"vuo-telemetry", required_argument, NULL, 0},
			{"vuo-loader", required_argument, NULL, 0},
			{"vuo-runner-pipe", required_argument, NULL, 0},
			{"vuo-continue-if-runner-dies", no_argument, NULL, 0},
			{"vuo-full", no_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		while((getopt_long(argc, argv, "", options, &optionIndex)) != -1)
		{
			switch(optionIndex)
			{
				case 0:	// "vuo-control"
					controlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(controlURL, optarg);
					break;
				case 1:	// "vuo-telemetry"
					telemetryURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(telemetryURL, optarg);
					break;
				case 2:	// "vuo-loader"
					if (loaderControlURL)
						free(loaderControlURL);
					loaderControlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(loaderControlURL, optarg);
					break;
				case 3:  // --vuo-runner-pipe
					runnerPipe = atoi(optarg);
					break;
				case 4:  // --vuo-continue-if-runner-dies
					continueIfRunnerDies = true;
					break;
				case 5: // "vuo-full"
					trialRestrictionsEnabled = false;
					break;
			}
		}
	}

	if (!loaderControlURL)
	{
		VUserLog("Error: Please specify a --vuo-loader URL.");
		return -1;
	}

	// Set up ZMQ connections.
	{
		ZMQLoaderControlContext = zmq_init(1);

		ZMQLoaderControl = zmq_socket(ZMQLoaderControlContext,ZMQ_REP);
		if(zmq_bind(ZMQLoaderControl,loaderControlURL))
		{
			VUserLog("The composition couldn't start because the composition loader couldn't establish communication to control the composition : %s", strerror(errno));
			free(loaderControlURL);
			return -1;
		}
		free(loaderControlURL);
	}

	// Launch control responder.
	dispatch_queue_t loaderControlQueue;
	dispatch_source_t loaderControlTimer;
	dispatch_semaphore_t loaderControlCanceledSemaphore;
	{
		loaderControlCanceledSemaphore = dispatch_semaphore_create(0);
		loaderControlQueue = dispatch_queue_create("org.vuo.runtime.loader", NULL);
		loaderControlTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,loaderControlQueue);
		dispatch_source_set_timer(loaderControlTimer, dispatch_walltime(NULL,0), NSEC_PER_SEC/1000, NSEC_PER_SEC/1000);
		dispatch_source_set_event_handler(loaderControlTimer, ^{

											  vuoMemoryBarrier();

											  zmq_pollitem_t items[]=
											  {
												  {ZMQLoaderControl,0,ZMQ_POLLIN,0},
											  };
											  int itemCount = 1;
											  long timeout = 1000;  // wait interruptably, to be able to cancel this dispatch source when the composition stops
											  zmq_poll(items,itemCount,timeout);
											  if(!(items[0].revents & ZMQ_POLLIN))
											  return;

											  enum VuoLoaderControlRequest control = (enum VuoLoaderControlRequest) vuoReceiveInt(ZMQLoaderControl, NULL);

											  switch (control)
											  {
												  case VuoLoaderControlRequestCompositionReplace:
												  {
													  char *dylibPath = vuoReceiveAndCopyString(ZMQLoaderControl, NULL);
													  char *resourceDylibPath = vuoReceiveAndCopyString(ZMQLoaderControl, NULL);
													  char *compositionDiff = vuoReceiveAndCopyString(ZMQLoaderControl, NULL);

													  bool ok = true;
													  ok = ok && loadResourceDylib(resourceDylibPath);
													  ok = ok && replaceComposition(dylibPath, compositionDiff);

													  free(dylibPath);
													  free(resourceDylibPath);

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
		bool isStoppedInitially = false;
		isStopped = &isStoppedInitially;
		while (isReplacing || ! *isStopped)  // Check isReplacing first, since isStopped is invalid for part of the time that isReplacing is true.
			VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
	}

	// Clean up ZMQ connections.
	{
		vuoMemoryBarrier();

		zmq_close(ZMQControl);

		dispatch_source_cancel(loaderControlTimer);
		dispatch_semaphore_wait(loaderControlCanceledSemaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(loaderControlCanceledSemaphore);
		dispatch_release(loaderControlTimer);
		dispatch_sync(loaderControlQueue, ^{
			zmq_close(ZMQLoaderControl);
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

	unloadResourceDylibs();

	return 0;
}

/**
 * Replaces the currently running composition (if any) with the given composition.
 *
 * This function takes ownership of @a updatedCompositionDiff and will free it on the next call.
 */
bool replaceComposition(const char *dylibPath, char *updatedCompositionDiff)
{
	isReplacing = true;

	bool isStopRequestedByComposition = false;
	char *serializedTelemetryState = NULL;
	char *serializedComposition = NULL;
	void *VuoCompositionFiniCallbackList = NULL;

	// Serialize and stop the old composition (if any).
	if (dylibHandle)
	{
		char **compositionDiff = (char**)dlsym(dylibHandle, "compositionDiff");
		if (! compositionDiff)
		{
			VUserLog("The composition couldn't be replaced because compositionDiff couldn't be found in the composition library : %s", dlerror());
			return false;
		}
		free(*compositionDiff);
		*compositionDiff = updatedCompositionDiff;

		typedef char * (* vuoSerializeType)(void);
		vuoSerializeType vuoSerialize = (vuoSerializeType) dlsym(dylibHandle, "vuoSerialize");
		if (! vuoSerialize)
		{
			VUserLog("The composition couldn't be replaced because vuoSerialize() couldn't be found in the composition library : %s", dlerror());
			return false;
		}

		serializedComposition = vuoSerialize();

		stopComposition();

		bool *isStopRequested = (bool *)dlsym(dylibHandle, "isStopRequested");
		if (! isStopRequested)
		{
			VUserLog("The composition couldn't be replaced because isStopRequested couldn't be found in the composition library : %s", dlerror());
			return false;
		}
		isStopRequestedByComposition = *isStopRequested;

		typedef char * (* vuoSerializePortsType)(void);
		vuoSerializePortsType vuoSerializeTelemetryState = (vuoSerializePortsType) dlsym(dylibHandle, "vuoSerializeTelemetryState");
		if (! vuoSerializeTelemetryState)
		{
			VUserLog("The composition couldn't be replaced because vuoSerializeTelemetryState() couldn't be found in the composition library : %s", dlerror());
			return false;
		}
		serializedTelemetryState = vuoSerializeTelemetryState();

		zmq_close(ZMQControl);
		ZMQControl = NULL;

		VuoFiniType *vuoFini = (VuoFiniType *)dlsym(dylibHandle, "vuoFini");
		if (! vuoFini)
		{
			VUserLog("The composition couldn't be replaced because vuoFini() couldn't be found in the composition library : %s", dlerror());
			return false;
		}
		vuoFini();

		VuoCompositionFiniCallbackList = *((void **)dlsym(dylibHandle, "VuoCompositionFiniCallbackList"));

		isStopped = NULL;

		dlclose(dylibHandle);
		dylibHandle = NULL;
	}

	// Start the new composition paused.
	{
		ZMQControlContext = zmq_init(1);

		ZMQControl = zmq_socket(ZMQControlContext,ZMQ_REQ);
		if (zmq_connect(ZMQControl,controlURL))
		{
			VUserLog("The composition couldn't be replaced because the composition loader couldn't establish communication to control the composition : %s", strerror(errno));
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

		isStopped = (bool *)dlsym(dylibHandle, "isStopped");
		if (! isStopped)
		{
			VUserLog("The composition couldn't be replaced because isStopped couldn't be found in '%s' : %s", dylibPath, dlerror());
			return false;
		}

		vuoInitInProcess(ZMQControlContext, controlURL, telemetryURL, true, runnerPid, runnerPipe, continueIfRunnerDies, trialRestrictionsEnabled, VuoCompositionFiniCallbackList);
	}

	// Unserialize the old composition's state (if any) into the new composition.
	char **compositionDiff = NULL;
	if (serializedComposition)
	{
		compositionDiff = (char **)dlsym(dylibHandle, "compositionDiff");
		if (! compositionDiff)
		{
			VUserLog("The composition couldn't be replaced because compositionDiff couldn't be found in '%s' : %s", dylibPath, dlerror());
			return false;
		}
		*compositionDiff = updatedCompositionDiff;

		typedef void (* vuoUnserializeType)(char *);
		vuoUnserializeType vuoUnserialize = (vuoUnserializeType) dlsym(dylibHandle, "vuoUnserialize");
		vuoUnserialize(serializedComposition);

		typedef void (* vuoUnserializePortsType)(char *);
		vuoUnserializePortsType vuoUnserializeTelemetryState = (vuoUnserializePortsType) dlsym(dylibHandle, "vuoUnserializeTelemetryState");
		vuoUnserializeTelemetryState(serializedTelemetryState);
		free(serializedTelemetryState);
	}

	// If the composition had a pending call to vuoStopComposition() when it was stopped, call it again.
	if (isStopRequestedByComposition)
	{
		typedef void (*vuoStopCompositionType)(void);
		vuoStopCompositionType vuoStopComposition = (vuoStopCompositionType) dlsym(dylibHandle, "vuoStopComposition");
		if (! vuoStopComposition)
		{
			VUserLog("The composition couldn't be replaced because vuoStopComposition couldn't be found in '%s' : %s", dylibPath, dlerror());
			return false;
		}
		vuoStopComposition();
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
	vuoInitMessageWithBool(&messages[1], true);
	vuoControlRequestSend(VuoControlRequestCompositionStop,messages,2);
	vuoControlReplyReceive(VuoControlReplyCompositionStopping);
}

/**
 * Loads the resource dylib at the given path and adds the resulting handle to the list of resource dylibs.
 *
 * If @c resourceDylibPath is the empty string, does nothing.
 */
bool loadResourceDylib(const char *resourceDylibPath)
{
	if (strlen(resourceDylibPath) == 0)
		return true;

	void *resourceDylibHandle = dlopen(resourceDylibPath, RTLD_NOW);
	if (! resourceDylibHandle)
	{
		VUserLog("The composition couldn't be replaced because the library '%s' couldn't be loaded : %s", resourceDylibPath, dlerror());
		return false;
	}

	if (resourceDylibHandlesSize == 0)
	{
		resourceDylibHandlesCapacity = 1;
		resourceDylibHandles = (void **)malloc(resourceDylibHandlesCapacity * sizeof(void *));
	}
	else if (resourceDylibHandlesSize == resourceDylibHandlesCapacity)
	{
		resourceDylibHandlesCapacity *= 2;
		void **newResourceDylibHandles = (void **)malloc(resourceDylibHandlesCapacity * sizeof(void *));
		for (int i = 0; i < resourceDylibHandlesSize; ++i)
			newResourceDylibHandles[i] = resourceDylibHandles[i];
		free(resourceDylibHandles);
		resourceDylibHandles = newResourceDylibHandles;
	}

	resourceDylibHandles[resourceDylibHandlesSize++] = resourceDylibHandle;

	return true;
}

/**
 * Unloads all resource dylibs.
 */
void unloadResourceDylibs(void)
{
	for (int i = 0; i < resourceDylibHandlesSize; ++i)
		dlclose(resourceDylibHandles[i]);
}
