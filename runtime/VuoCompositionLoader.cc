/**
 * @file
 * VuoCompositionLoader implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <dlfcn.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoTelemetry.h"

extern "C"
{
/**
 * Private API function in libdispatch.
 */
extern void _dispatch_main_queue_callback_4CF(mach_msg_header_t *msg);
}

void *ZMQLoaderControlContext = NULL;  ///< The context for initializing sockets to control the composition loader.
void *ZMQLoaderControl = NULL;  ///< The socket for controlling the composition loader.
void *ZMQControlContext = NULL;  ///< The context for initializing sockets to control the composition.
void *ZMQControl = NULL;  ///< The socket for controlling the composition.
char *controlURL = NULL;  ///< The URL that the composition will use to initialize its control socket.
char *telemetryURL = NULL;  ///< The URL that the composition will use to initialize its telemetry socket.
bool *isStopped = NULL;  ///< True if the composition has stopped.
bool *isPaused = NULL;  ///< True if the composition is paused.
bool isReplacing = false;  ///< True if the composition is in the process of being replaced.
void *dylibHandle = NULL;  ///< A handle to the running composition.
void **resourceDylibHandles = NULL;  ///< A list of handles to the running composition's resources.
size_t resourceDylibHandlesSize = 0;  ///< The number of items in @c resourceDylibHandles.
size_t resourceDylibHandlesCapacity = 0;  ///< The number of items that @c resourceDylibHandlesCapacity can currently hold.

void replaceComposition(const char *dylibPath, const char *compositionDiff);
void stopComposition(void);
void pauseComposition(void);
void unpauseComposition(void);
void loadResourceDylib(const char *resourceDylibPath);
void unloadResourceDylibs(void);

/**
 * Sends a control reply message to the process controlling this composition loader.
 */
void vuoLoaderControlReplySend(enum VuoLoaderControlReply reply, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoLoaderControl",ZMQLoaderControl,reply,messages,messageCount,false);
}

/**
 * Sends a control request message to the running composition.
 */
void vuoControlRequestSend(enum VuoControlRequest request, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoControl",ZMQControl,request,messages,messageCount,false);
}

/**
 * Receives a control reply message from the running composition, and checks that it is the expected reply.
 */
void vuoControlReplyReceive(enum VuoControlReply expectedReply)
{
	int reply = vuoReceiveInt(ZMQControl);
	if (reply != expectedReply)
		fprintf(stderr, "VuoControl message received unexpected reply (received %u, expected %u)\n", reply, expectedReply);
}

/**
 * Starts the composition loader.
 */
int main(int argc, char **argv)
{
	char *loaderControlURL = NULL;
	bool isPausedOnStart = false;
	isPaused = &isPausedOnStart;

	// Parse commandline arguments.
	{
		static struct option options[] = {
			{"vuo-control", required_argument, NULL, 0},
			{"vuo-telemetry", required_argument, NULL, 0},
			{"vuo-pause", no_argument, NULL, 0},
			{"vuo-loader", required_argument, NULL, 0},
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
				case 2: // "vuo-pause"
					isPausedOnStart = true;
					break;
				case 3:	// "vuo-loader"
					loaderControlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(loaderControlURL, optarg);
					break;
			}
		}
	}

	// Set up ZMQ connections.
	{
		ZMQLoaderControlContext = zmq_init(1);

		ZMQLoaderControl = zmq_socket(ZMQLoaderControlContext,ZMQ_REP);
		if(zmq_bind(ZMQLoaderControl,loaderControlURL))
		{
			fprintf(stderr, "VuoLoaderControl: bind failed (%s).\n", loaderControlURL);
			return 1;
		}
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

											  enum VuoLoaderControlRequest control = (enum VuoLoaderControlRequest) vuoReceiveInt(ZMQLoaderControl);

											  switch (control)
											  {
												  case VuoLoaderControlRequestCompositionReplace:
												  {
													  char *dylibPath = vuoReceiveAndCopyString(ZMQLoaderControl);
													  char *resourceDylibPath = vuoReceiveAndCopyString(ZMQLoaderControl);
													  char *compositionDiff = vuoReceiveAndCopyString(ZMQLoaderControl);
													  loadResourceDylib(resourceDylibPath);
													  replaceComposition(dylibPath, compositionDiff);
													  free(dylibPath);
													  free(resourceDylibPath);
													  free(compositionDiff);

													  vuoLoaderControlReplySend(VuoLoaderControlReplyCompositionReplaced,NULL,0);
													  break;
												  }
												  default:
												  {
													  fprintf(stderr, "VuoLoaderControl: got unknown request %d\n", control);
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
		{
			_dispatch_main_queue_callback_4CF(0);
			usleep(10000);
		}
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

		typedef void(* vuoFiniType)(void);
		vuoFiniType vuoFini = (vuoFiniType) dlsym(dylibHandle, "vuoFini");
		if (! vuoFini)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find function `vuoFini`: %s\n", dlerror());
			return 1;
		}
		vuoFini();
	}

	unloadResourceDylibs();

	return 0;
}

/**
 * Replaces the currently running composition (if any) with the given composition.
 */
void replaceComposition(const char *dylibPath, const char *updatedCompositionDiff)
{
	isReplacing = true;

	bool isPausedOnStart = *isPaused;  // Store isPaused, since it will become invalid when the old composition is unloaded.

	// Serialize and stop the old composition (if any).
	char *serializedComposition = NULL;
	if (dylibHandle)
	{
		const char **compositionDiff = (const char**)dlsym(dylibHandle, "compositionDiff");
		if (! compositionDiff)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find variable `compositionDiff`: %s\n", dlerror());
			return;
		}
		*compositionDiff = updatedCompositionDiff;

		if (! *isPaused)
			pauseComposition();

		typedef char * (* vuoSerializeType)(void);
		vuoSerializeType vuoSerialize = (vuoSerializeType) dlsym(dylibHandle, "vuoSerialize");
		if (! vuoSerialize)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find function `vuoSerialize`: %s\n", dlerror());
			return;
		}

		serializedComposition = vuoSerialize();

		stopComposition();

		zmq_close(ZMQControl);
		ZMQControl = NULL;

		typedef void(* vuoFiniType)(void);

		vuoFiniType vuoFini = (vuoFiniType)dlsym(dylibHandle, "vuoFini");
		if (! vuoFini)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find function `vuoFini`: %s\n", dlerror());
			return;
		}
		vuoFini();

		isStopped = NULL;
		isPaused = NULL;

		dlclose(dylibHandle);
		dylibHandle = NULL;
	}

	// Start the new composition paused.
	{
		ZMQControlContext = zmq_init(1);

		ZMQControl = zmq_socket(ZMQControlContext,ZMQ_REQ);
		if (zmq_connect(ZMQControl,controlURL))
		{
			fprintf(stderr, "VuoControl: connect failed (%s).\n", controlURL);
			return;
		}

		typedef void(* vuoInitInProcessType)(void *_ZMQContext, const char *controlURL, const char *telemetryURL, bool _isPaused);
		vuoInitInProcessType vuoInitInProcess = NULL;

		dylibHandle = dlopen(dylibPath, RTLD_NOW);
		if (! dylibHandle)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't load dylib: %s\n", dlerror());
			return;
		}

		vuoInitInProcess = (vuoInitInProcessType)dlsym(dylibHandle, "vuoInitInProcess");
		if (! vuoInitInProcess)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find function `vuoInitInProcess`: %s\n", dlerror());
			return;
		}

		isStopped = (bool *)dlsym(dylibHandle, "isStopped");
		if (! isStopped)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find variable `isStopped`: %s\n", dlerror());
			return;
		}

		isPaused = (bool *)dlsym(dylibHandle, "isPaused");
		if (! isPaused)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find variable `isPaused`: %s\n", dlerror());
			return;
		}

		vuoInitInProcess(ZMQControlContext, controlURL, telemetryURL, true);
	}

	// Unserialize the old composition's state (if any) into the new composition.
	const char **compositionDiff = NULL;
	if (serializedComposition)
	{
		compositionDiff = (const char **)dlsym(dylibHandle, "compositionDiff");
		if (! compositionDiff)
		{
			fprintf(stderr, "VuoCompositionLoader: couldn't find variable `compositionDiff`: %s\n", dlerror());
			return;
		}
		*compositionDiff = updatedCompositionDiff;

		typedef void (* vuoUnserializeType)(char *);
		vuoUnserializeType vuoUnserialize = (vuoUnserializeType) dlsym(dylibHandle, "vuoUnserialize");

		vuoUnserialize(serializedComposition);
	}

	// Unpause the new composition (if needed).
	if (! isPausedOnStart)
	{
		unpauseComposition();
	}

	// Reset the composition's `compositionDiff` back to NULL, since this function's caller is about to free its pointee.
	if (serializedComposition)
	{
		*compositionDiff = NULL;
	}

	isReplacing = false;
}

/**
 * Sends a control request to the composition telling it to stop.
 */
void stopComposition(void)
{
	vuoMemoryBarrier();

	vuoControlRequestSend(VuoControlRequestCompositionStop,NULL,0);
	vuoControlReplyReceive(VuoControlReplyCompositionStopping);
}

/**
 * Sends a control request to the composition telling it to pause.
 */
void pauseComposition(void)
{
	vuoMemoryBarrier();

	vuoControlRequestSend(VuoControlRequestCompositionPause,NULL,0);
	vuoControlReplyReceive(VuoControlReplyCompositionPaused);
}

/**
 * Sends a control request to the composition telling it to unpause.
 */
void unpauseComposition(void)
{
	vuoMemoryBarrier();

	vuoControlRequestSend(VuoControlRequestCompositionUnpause,NULL,0);
	vuoControlReplyReceive(VuoControlReplyCompositionUnpaused);
}

/**
 * Loads the resource dylib at the given path and adds the resulting handle to the list of resource dylibs.
 *
 * If @c resourceDylibPath is the empty string, does nothing.
 */
void loadResourceDylib(const char *resourceDylibPath)
{
	if (strlen(resourceDylibPath) == 0)
		return;

	void *resourceDylibHandle = dlopen(resourceDylibPath, RTLD_NOW);
	if (! resourceDylibHandle)
	{
		fprintf(stderr, "VuoCompositionLoader: couldn't load resource dylib: %s\n", dlerror());
		return;
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
}

/**
 * Unloads all resource dylibs.
 */
void unloadResourceDylibs(void)
{
	for (int i = 0; i < resourceDylibHandlesSize; ++i)
		dlclose(resourceDylibHandles[i]);
}
