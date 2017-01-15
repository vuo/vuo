/**
 * @file
 * VuoRuntime implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#include <libgen.h> // for dirname()
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include "VuoRuntime.h"
#include "VuoEventLoop.h"
#include "VuoHeap.h"

extern "C"
{

bool hasZMQConnection = false;   ///< True if the @ref ZMQControl and @ref ZMQTelemetry sockets are connected to something.

static dispatch_queue_t VuoControlQueue;	///< Dispatch queue for protecting access to the @c ZMQControl socket.
static dispatch_queue_t VuoTelemetryQueue;	///< Dispatch queue for protecting access to the @c ZMQTelemetry socket.

static void *ZMQContext;	///< The context used to initialize sockets.
static void *ZMQControl;	///< The control socket. Use only on VuoControlQueue.
static void *ZMQSelfReceive = 0;	///< Used to break out of a ZMQControl poll.
static void *ZMQSelfSend = 0;		///< Used to break out of a ZMQControl poll.
static void *ZMQTelemetry = 0;		///< The telemetry socket. Use only on VuoTelemetryQueue.

static bool hasBeenUnpaused;  ///< True if node execution was unpaused initially, or if it has since been unpaused.
bool isStopped; ///< True if composition execution has stopped.
bool isStopRequested;  ///< True if vuoStopComposition() has been called.
#ifndef DOXYGEN
bool *VuoTrialRestrictionsEnabled;	///< If true, some nodes may restrict how they can be used.
#endif

static dispatch_source_t telemetryTimer;  ///< Timer for sending telemetry messages.
static dispatch_source_t controlTimer;  ///< Timer for receiving control messages.
static dispatch_source_t waitForStopTimer = NULL;  ///< Timer for checking if the runner will stop the composition.
static dispatch_semaphore_t telemetryCanceledSemaphore;  ///< Signaled when telemetry events are no longer being processed.
static dispatch_semaphore_t controlCanceledSemaphore;  ///< Signaled when control events are no longer being processed.
static dispatch_semaphore_t waitForStopCanceledSemaphore;  ///< Signaled when no longer checking if the runner will stop the composition.

static bool isSendingAllTelemetry = false;  ///< True if all telemetry should be sent.
static bool isSendingEventTelemetry = false;  ///< True if all telemetry about events (not including data) should be sent.
static set<string> portsSendingDataTelemetry;  ///< Port identifiers for which data-and-event telemetry should be sent.

static pid_t runnerPid;  ///< Process ID of the runner that started the composition.
static void stopComposition(bool isBeingReplaced);
void vuoStopComposition(void);
char * getInputPortString(char *portIdentifier, bool shouldUseInterprocessSerialization);
char * getOutputPortString(char *portIdentifier, bool shouldUseInterprocessSerialization);
static char * getInputPortSummary(char *portIdentifier);
static char * getOutputPortSummary(char *portIdentifier);

typedef void (*VuoCompositionFiniCallback)(void);	///< Callback prototype.
typedef std::list<VuoCompositionFiniCallback> VuoCompositionFiniCallbackListType;	///< Type for fini callback list.
VuoCompositionFiniCallbackListType *VuoCompositionFiniCallbackList;	///< Fini callbacks to invoke upon composition shutdown.  Non-static, so that VuoCompositionLoader can fetch and restore it during live-coding reloads.
static dispatch_queue_t VuoCompositionFiniCallbackQueue = NULL;	///< Serializes access to the list of fini callbacks.

static dispatch_queue_t VuoCompositionStopQueue = NULL;	///< Ensures stops happen serially.


/**
 * @threadQueue{VuoControlQueue}
 */
static void vuoControlReplySend(enum VuoControlReply reply, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoControl",ZMQControl,reply,messages,messageCount,false,NULL);
}

/**
 * @threadAny
 */
void vuoTelemetrySend(enum VuoTelemetry type, zmq_msg_t *messages, unsigned int messageCount)
{
	/// @todo https://b33p.net/kosada/node/5567
	if (!ZMQTelemetry)
		return;

	dispatch_sync(VuoTelemetryQueue, ^{
		vuoMemoryBarrier();
		vuoSend("VuoTelemetry",ZMQTelemetry,type,messages,messageCount,true,NULL);
	});
}


//@{
/**
 * @internal
 * Defined in VuoRuntimeHelper. Declared 'extern' here to avoid duplicate symbol errors between VuoRuntime and VuoRuntimeHelper.
 */
extern bool isPaused;
extern char *compositionDiff;
//@}

//@{
/**
 * @internal
 * Defined in the composition's generated code.
 */
extern void vuoSetup(void);
extern void vuoCleanup(void);
extern void vuoInstanceInit(void);
extern void vuoInstanceFini(void);
extern void vuoInstanceTriggerStart(void);
extern void vuoInstanceTriggerStop(void);
extern void vuoSetInputPortValue(char *portIdentifier, char *valueAsString);
extern void fireTriggerPortEvent(char *portIdentifier);
extern char * vuoGetPortValue(char *portIdentifier, int serializationType);
extern unsigned int getPublishedInputPortCount(void);
extern unsigned int getPublishedOutputPortCount(void);
extern char ** getPublishedInputPortNames(void);
extern char ** getPublishedOutputPortNames(void);
extern char ** getPublishedInputPortTypes(void);
extern char ** getPublishedOutputPortTypes(void);
extern char ** getPublishedInputPortDetails(void);
extern char ** getPublishedOutputPortDetails(void);
extern void firePublishedInputPortEvent(char *name);
extern void setPublishedInputPortValue(char *portIdentifier, char *valueAsString);
extern char * getPublishedInputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization);
extern char * getPublishedOutputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization);
extern void VuoHeap_report();
//@}


/**
 * Parses command-line arguments, then calls @c vuoInitInProcess().
 */
void vuoInit(int argc, char **argv)
{
	bool doAppInit = false;
	char *controlURL = NULL;
	char *telemetryURL = NULL;
	bool _isPaused = false;
	pid_t _runnerPid = getppid();
	int runnerPipe = -1;
	bool continueIfRunnerDies = false;
	bool doPrintHelp = false;
	bool doPrintLicenses = false;
	bool trialRestrictionsEnabled = false;

	// parse commandline arguments
	{
		int getoptArgC = 0;
		char **getoptArgV = (char **)malloc(sizeof(char *) * argc);
		for(int i = 0; i < argc; ++i)
		{
			// Don't pass the OS X Process Serial Number argument to getopt, since it can't handle long arguments with a single hyphen.
			if (strncmp(argv[i], "-psn_", 5) == 0)
			{
				// Since we have a process serial number, we can assume this is an exported app being invoked by LaunchServices.
				// Therefore we need to initialize the app (so it shows up in the dock).
				doAppInit = true;
				continue;
			}

			getoptArgV[getoptArgC++] = argv[i];
		}

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"vuo-control", required_argument, NULL, 0},
			{"vuo-telemetry", required_argument, NULL, 0},
			{"vuo-pause", no_argument, NULL, 0},
			{"vuo-loader", required_argument, NULL, 0},
			{"vuo-runner-pipe", required_argument, NULL, 0},
			{"vuo-continue-if-runner-dies", no_argument, NULL, 0},
			{"vuo-licenses", no_argument, NULL, 0},
			{"vuo-trial", no_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		while((getopt_long(getoptArgC, getoptArgV, "", options, &optionIndex)) != -1)
		{
			switch(optionIndex)
			{
				case 0:  // --help
					doPrintHelp = true;
					break;
				case 1:	 // --vuo-control
					if (controlURL)
						free(controlURL);
					controlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(controlURL, optarg);
					break;
				case 2:	 // --vuo-telemetry
					if (telemetryURL)
						free(telemetryURL);
					telemetryURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(telemetryURL, optarg);
					break;
				case 3:  // --vuo-pause
					_isPaused = true;
					break;
				case 4:  // --vuo-loader (ignored, but added here to avoid "unrecognized option" warning)
					break;
				case 5:  // --vuo-runner-pipe
					runnerPipe = atoi(optarg);
					break;
				case 6:  // --vuo-continue-if-runner-dies
					continueIfRunnerDies = true;
					break;
				case 7:  // --vuo-licenses
					doPrintLicenses = true;
					break;
				case 8:  // --vuo-trial
					trialRestrictionsEnabled = true;
					break;
			}
		}
		free(getoptArgV);
	}

	if (doPrintHelp)
	{
		printf("Usage: %s [options]\n"
			   "Options:\n"
			   "  --help                                   Display this information.\n"
			   "  --vuo-licenses                           Display license information.\n",
			   argv[0]);

		exit(0);
	}
	else if (doPrintLicenses)
	{
		printf("This composition may include software licensed under the following terms:\n\n");

		// Get the exported executable path.
		char rawExecutablePath[PATH_MAX+1];
		uint32_t size = sizeof(rawExecutablePath);
		_NSGetExecutablePath(rawExecutablePath, &size);

		char cleanedExecutablePath[PATH_MAX+1];
		realpath(rawExecutablePath, cleanedExecutablePath);

		// Derive the path of the app bundle's "Licenses" directory from its executable path.
		char executableDir[PATH_MAX+1];
		strcpy(executableDir, dirname(cleanedExecutablePath));

		const char *licensesPathFromExecutable = "/../Frameworks/Vuo.framework/Versions/" VUO_VERSION_STRING "/Documentation/Licenses";
		char rawLicensesPath[strlen(executableDir)+strlen(licensesPathFromExecutable)+1];
		strcpy(rawLicensesPath, executableDir);
		strcat(rawLicensesPath, licensesPathFromExecutable);

		char cleanedLicensesPath[PATH_MAX+1];
		realpath(rawLicensesPath, cleanedLicensesPath);

		bool foundLicenses = false;
		if (access(cleanedLicensesPath, 0) == 0)
		{
			DIR *dirp = opendir(cleanedLicensesPath);
			struct dirent *dp;
			while ((dp = readdir(dirp)) != NULL)
			{
				if (dp->d_name[0] == '.')
					continue;

				printf("=== %s =====================================================\n\n",dp->d_name);

				char licensePath[strlen(cleanedLicensesPath) + dp->d_namlen + 2];
				strcpy(licensePath, cleanedLicensesPath);
				strcat(licensePath, "/");
				strcat(licensePath, dp->d_name);

				int fd = open(licensePath, O_RDONLY);
				char data[1024];
				int bytesRead;
				while((bytesRead = read(fd, data, 1024)) > 0)
					write(1, data, bytesRead);
				close(fd);

				printf("\n\n\n");
				foundLicenses = true;
			}
			closedir(dirp);
		}

		if (!foundLicenses)
			printf("(No license information found.)\n");

		free(controlURL);
		free(telemetryURL);
		exit(0);
	}

	VuoCompositionFiniCallbackQueue = dispatch_queue_create("vuo.runtime.finiCallback", NULL);
	VuoCompositionFiniCallbackList = new VuoCompositionFiniCallbackListType;

	if (doAppInit)
	{
		typedef void (*vuoAppInitType)(void);
		vuoAppInitType vuoAppInit = (vuoAppInitType) dlsym(RTLD_SELF, "VuoApp_init");
		if (vuoAppInit)
			vuoAppInit();
	}

	vuoInitInProcess(NULL, controlURL, telemetryURL, _isPaused, _runnerPid, runnerPipe, continueIfRunnerDies, trialRestrictionsEnabled, VuoCompositionFiniCallbackList);
	free(controlURL);
	free(telemetryURL);
}

/**
 * If `type` is an enum, merges the allowed enum values into `details` and returns it (caller responsible for freeing).
 *
 * Otherwise, returns NULL.
 */
char *VuoRuntime_mergeEnumDetails(string type, char *details)
{
	string allowedValuesFunctionName = type + "_getAllowedValues";
	typedef void *(*allowedValuesFunctionType)(void);
	allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_SELF, allowedValuesFunctionName.c_str());
	if (!allowedValuesFunction)
		return NULL;

	string getJsonFunctionName = type + "_getJson";
	typedef json_object *(*getJsonFunctionType)(int);
	getJsonFunctionType getJsonFunction = (getJsonFunctionType)dlsym(RTLD_SELF, getJsonFunctionName.c_str());
	if (!getJsonFunction)
		return NULL;

	string summaryFunctionName = type + "_getSummary";
	typedef char *(*summaryFunctionType)(int);
	summaryFunctionType summaryFunction = (summaryFunctionType)dlsym(RTLD_SELF, summaryFunctionName.c_str());
	if (!summaryFunction)
		return NULL;

	string listCountFunctionName = "VuoListGetCount_" + type;
	typedef unsigned long (*listCountFunctionType)(void *);
	listCountFunctionType listCountFunction = (listCountFunctionType)dlsym(RTLD_SELF, listCountFunctionName.c_str());
	if (!listCountFunction)
		return NULL;

	string listValueFunctionName = "VuoListGetValue_" + type;
	typedef int (*listValueFunctionType)(void *, unsigned long);
	listValueFunctionType listValueFunction = (listValueFunctionType)dlsym(RTLD_SELF, listValueFunctionName.c_str());
	if (!listValueFunction)
		return NULL;

	json_object *detailsJson = json_tokener_parse(details);
	if (!detailsJson)
		return NULL;

	void *allowedValues = allowedValuesFunction();
	VuoRetain(allowedValues);
	unsigned long listCount = listCountFunction(allowedValues);
	json_object *menuItems = json_object_new_array();
	for (unsigned long i = 1; i <= listCount; ++i)
	{
		int value = listValueFunction(allowedValues, i);
		json_object *js = getJsonFunction(value);
		if (!json_object_is_type(js, json_type_string))
			continue;
		const char *key = json_object_get_string(js);
		char *summary = summaryFunction(value);

		json_object *menuItem = json_object_new_object();
		json_object_object_add(menuItem, "value", json_object_new_string(key));
		json_object_object_add(menuItem, "name", json_object_new_string(summary));
		json_object_array_add(menuItems, menuItem);

		free(summary);
	}
	VuoRelease(allowedValues);

	json_object_object_add(detailsJson, "menuItems", menuItems);
	char *newDetails = strdup(json_object_to_json_string(detailsJson));
	json_object_put(detailsJson);

	return newDetails;
}

/**
 * Sets up ZMQ control and telemetry sockets, then calls the generated function @c vuoSetup().
 * If the composition is not paused, also calls @c vuoInstanceInit() and @c vuoInstanceTriggerStart().
 */
void vuoInitInProcess(void *_ZMQContext, const char *controlURL, const char *telemetryURL, bool _isPaused, pid_t _runnerPid,
					  int runnerPipe, bool continueIfRunnerDies, bool trialRestrictionsEnabled, void *_VuoCompositionFiniCallbackList)
{
	if (controlURL && telemetryURL)
	{
		hasZMQConnection = true;
		ZMQContext = (_ZMQContext ? _ZMQContext : zmq_init(1));

		ZMQControl = zmq_socket(ZMQContext,ZMQ_REP);
		ZMQTelemetry = zmq_socket(ZMQContext,ZMQ_PUB);

		bool connectionError = false;

		if(zmq_bind(ZMQControl,controlURL))
		{
			VUserLog("The composition couldn't start because it couldn't establish communication to be controlled by the runner : %s", strerror(errno));
			connectionError = true;
		}

		/// @todo When running multiple compositions in the same process, do we need separate pairs? https://b33p.net/kosada/node/10374
		ZMQSelfReceive = zmq_socket(ZMQContext, ZMQ_PAIR);
		if (zmq_bind(ZMQSelfReceive, "inproc://vuo-runtime-self") != 0)
		{
			VUserLog("Couldn't bind self-receive socket: %s (%d)", strerror(errno), errno);
			connectionError = true;
		}

		ZMQSelfSend = zmq_socket(ZMQContext, ZMQ_PAIR);
		if (zmq_connect(ZMQSelfSend, "inproc://vuo-runtime-self") != 0)
		{
			VUserLog("Couldn't connect self-send socket: %s (%d)", strerror(errno), errno);
			connectionError = true;
		}

		if(zmq_bind(ZMQTelemetry,telemetryURL))
		{
			VUserLog("The composition couldn't start because it couldn't establish communication to be listened to by the runner : %s", strerror(errno));
			connectionError = true;
		}

		if (connectionError)
		{
			zmq_close(ZMQSelfSend);
			ZMQSelfSend = NULL;
			zmq_close(ZMQSelfReceive);
			ZMQSelfReceive = NULL;
			zmq_close(ZMQControl);
			ZMQControl = NULL;
			zmq_close(ZMQTelemetry);
			ZMQTelemetry = NULL;
			return;
		}

		VuoTelemetryQueue = dispatch_queue_create("org.vuo.runtime.telemetry", NULL);
	}

	VuoControlQueue = dispatch_queue_create("org.vuo.runtime.control", NULL);

	hasBeenUnpaused = false;
	isPaused = _isPaused;
	isStopped = false;
	isStopRequested = false;

	if (!VuoCompositionFiniCallbackQueue)
		VuoCompositionFiniCallbackQueue = dispatch_queue_create("vuo.runtime.finiCallback", NULL);

	VuoCompositionFiniCallbackList = (VuoCompositionFiniCallbackListType *)_VuoCompositionFiniCallbackList;
	if (!_VuoCompositionFiniCallbackList)
		VuoCompositionFiniCallbackList = new VuoCompositionFiniCallbackListType;

	VuoCompositionStopQueue = dispatch_queue_create("vuo.runtime.stop", NULL);

	// Set the `VuoTrialRestrictionsEnabled` global, and protect it.
	{
		VuoTrialRestrictionsEnabled = NULL;
		int pagesize = sysconf(_SC_PAGE_SIZE);
		if (pagesize == -1)
		{
//			VLog("Error: Couldn't configure VuoTrialRestrictionsEnabled: %s", strerror(errno));
		}
		else
		{
			VuoTrialRestrictionsEnabled = (bool *)mmap(NULL, pagesize, PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
			if (VuoTrialRestrictionsEnabled == MAP_FAILED)
			{
//				VLog("Error: Couldn't configure VuoTrialRestrictionsEnabled: %s", strerror(errno));
			}
			else
			{
				*VuoTrialRestrictionsEnabled = trialRestrictionsEnabled;
				if (mprotect(VuoTrialRestrictionsEnabled, pagesize, PROT_READ) == -1)
				{
//					VLog("Error: Couldn't configure VuoTrialRestrictionsEnabled: %s", strerror(errno));
				}
			}
		}
	}


	runnerPid = _runnerPid;
	waitForStopCanceledSemaphore = dispatch_semaphore_create(0);

	// set up composition
	{
		vuoSetup();

		if (! isPaused)
		{
			// Wait to call vuoInstanceInit() until the first time the composition enters an unpaused state.
			// If vuoInstanceInit() were called immediately when the composition is started in a paused state(),
			// then the vuo.event.fireOnStart node's fired event would be ignored.

			hasBeenUnpaused = true;
			__block bool initDone = false;
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   vuoInstanceInit();
							   vuoInstanceTriggerStart();
							   initDone = true;
							   VuoEventLoop_break();
						   });
			while (! initDone)
				VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
		}
	}


	// launch VuoTelemetryStats probe
	if (hasZMQConnection)
	{
		telemetryCanceledSemaphore = dispatch_semaphore_create(0);
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
		SInt32 macMinorVersion;
		Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
		unsigned long mask = (macMinorVersion >= 9 ? 0x1 : 0x0);  // DISPATCH_TIMER_STRICT
		telemetryTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, mask, queue);
		dispatch_source_set_timer(telemetryTimer, dispatch_walltime(NULL, 0), NSEC_PER_SEC/1000, NSEC_PER_SEC/1000);
		dispatch_source_set_event_handler(telemetryTimer, ^{

			struct rusage r;
			if(getrusage(RUSAGE_SELF,&r))
			{
				VUserLog("The composition couldn't get the information to send for VuoTelemetryStats : %s", strerror(errno));
				return;
			}

			zmq_msg_t messages[2];

			{
				uint64_t utime = r.ru_utime.tv_sec*USEC_PER_SEC+r.ru_utime.tv_usec;
				zmq_msg_init_size(&messages[0], sizeof utime);
				memcpy(zmq_msg_data(&messages[0]), &utime, sizeof utime);
			}

			{
				uint64_t stime = r.ru_stime.tv_sec*USEC_PER_SEC+r.ru_stime.tv_usec;
				zmq_msg_init_size(&messages[1], sizeof stime);
				memcpy(zmq_msg_data(&messages[1]), &stime, sizeof stime);
			}

			vuoTelemetrySend(VuoTelemetryStats, messages, 2);

		});
		dispatch_source_set_cancel_handler(telemetryTimer, ^{
			dispatch_semaphore_signal(telemetryCanceledSemaphore);
		});
		dispatch_resume(telemetryTimer);
	}


	// launch control responder
	if (hasZMQConnection)
	{
		controlCanceledSemaphore = dispatch_semaphore_create(0);
		controlTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,VuoControlQueue);
		dispatch_source_set_timer(controlTimer, dispatch_walltime(NULL,0), NSEC_PER_SEC/1000, NSEC_PER_SEC/1000);
		dispatch_source_set_event_handler(controlTimer, ^{

			vuoMemoryBarrier();

			zmq_pollitem_t items[]=
			{
				{ZMQControl,0,ZMQ_POLLIN,0},
				{ZMQSelfReceive,0,ZMQ_POLLIN,0},
			};
			int itemCount = 2;
			long timeout = -1;  // Wait forever (VuoStopComposition will send a message ZMQSelfReceive when it's time to stop).
			zmq_poll(items,itemCount,timeout);
			if(!(items[0].revents & ZMQ_POLLIN))
				return;

			enum VuoControlRequest control = (enum VuoControlRequest) vuoReceiveInt(ZMQControl, NULL);

			switch (control)
			{
				case VuoControlRequestSlowHeartbeat:
				{
					dispatch_source_set_timer(telemetryTimer, dispatch_walltime(NULL, 0), NSEC_PER_SEC/2, NSEC_PER_SEC/100);
					vuoControlReplySend(VuoControlReplyHeartbeatSlowed,NULL,0);
					break;
				}
				case VuoControlRequestCompositionStop:
				{
					int timeoutInSeconds = vuoReceiveInt(ZMQControl, NULL);
					bool isBeingReplaced = vuoReceiveBool(ZMQControl, NULL);

					if (timeoutInSeconds >= 0)
					{
						// If the composition is not stopped within the timeout, kill its process.
						dispatch_after(dispatch_time(DISPATCH_TIME_NOW, timeoutInSeconds * NSEC_PER_SEC),
									   dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							if (VuoEventLoop_mayBeTerminated())
							{
								vuoControlReplySend(VuoControlReplyCompositionStopping,NULL,0);
								zmq_close(ZMQControl);  // wait until message fully sends
								kill(getpid(), SIGKILL);
							}
						});
					}

					if (! isBeingReplaced)
					{
						free(compositionDiff);
						compositionDiff = NULL;
					}

					stopComposition(isBeingReplaced);

					if (timeoutInSeconds >= 0)
					{
						VUOLOG_PROFILE_BEGIN(mainQueue);
						dispatch_sync(dispatch_get_main_queue(), ^{
							VUOLOG_PROFILE_END(mainQueue);
							VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
						});
						VuoHeap_report();
					}

					vuoControlReplySend(VuoControlReplyCompositionStopping,NULL,0);

					break;
				}
				case VuoControlRequestCompositionPause:
				{
					isPaused = true;
					vuoInstanceTriggerStop();
					vuoControlReplySend(VuoControlReplyCompositionPaused,NULL,0);
					break;
				}
				case VuoControlRequestCompositionUnpause:
				{
					isPaused = false;

					if (! hasBeenUnpaused)
					{
						hasBeenUnpaused = true;
						vuoInstanceInit();
					}
					vuoInstanceTriggerStart();

					vuoControlReplySend(VuoControlReplyCompositionUnpaused,NULL,0);
					break;
				}
				case VuoControlRequestInputPortValueRetrieve:
				{
					bool shouldUseInterprocessSerialization = vuoReceiveBool(ZMQControl, NULL);
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *valueAsString = getInputPortString(portIdentifier, shouldUseInterprocessSerialization);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], valueAsString);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyInputPortValueRetrieved,messages,1);
					break;
				}
				case VuoControlRequestOutputPortValueRetrieve:
				{
					bool shouldUseInterprocessSerialization = vuoReceiveBool(ZMQControl, NULL);
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *valueAsString = getOutputPortString(portIdentifier, shouldUseInterprocessSerialization);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], valueAsString);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyOutputPortValueRetrieved,messages,1);
					break;
				}
				case VuoControlRequestInputPortSummaryRetrieve:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *summary = getInputPortSummary(portIdentifier);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], summary);
					free(summary);
					vuoControlReplySend(VuoControlReplyInputPortSummaryRetrieved,messages,1);
					break;
				}
				case VuoControlRequestOutputPortSummaryRetrieve:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *summary = getOutputPortSummary(portIdentifier);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], summary);
					free(summary);
					vuoControlReplySend(VuoControlReplyOutputPortSummaryRetrieved,messages,1);
					break;
				}
				case VuoControlRequestTriggerPortFireEvent:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					fireTriggerPortEvent(portIdentifier);
					free(portIdentifier);
					vuoControlReplySend(VuoControlReplyTriggerPortFiredEvent,NULL,0);
					break;
				}
				case VuoControlRequestInputPortValueModify:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *valueAsString = vuoReceiveAndCopyString(ZMQControl, NULL);
					vuoSetInputPortValue(portIdentifier, valueAsString);
					free(portIdentifier);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyInputPortValueModified,NULL,0);
					break;
				}
				case VuoControlRequestPublishedInputPortNamesRetrieve:
				{
					int count = getPublishedInputPortCount();
					char **names = getPublishedInputPortNames();

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
						vuoInitMessageWithString(&messages[i], names[i]);

					vuoControlReplySend(VuoControlReplyPublishedInputPortNamesRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedOutputPortNamesRetrieve:
				{
					int count = getPublishedOutputPortCount();
					char **names = getPublishedOutputPortNames();

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
						vuoInitMessageWithString(&messages[i], names[i]);

					vuoControlReplySend(VuoControlReplyPublishedOutputPortNamesRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedInputPortTypesRetrieve:
				{
					int count = getPublishedInputPortCount();
					char **names = getPublishedInputPortTypes();

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
						vuoInitMessageWithString(&messages[i], names[i]);

					vuoControlReplySend(VuoControlReplyPublishedInputPortTypesRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedOutputPortTypesRetrieve:
				{
					int count = getPublishedOutputPortCount();
					char **names = getPublishedOutputPortTypes();

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
						vuoInitMessageWithString(&messages[i], names[i]);

					vuoControlReplySend(VuoControlReplyPublishedOutputPortTypesRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedInputPortDetailsRetrieve:
				{
					int count = getPublishedInputPortCount();
					char **types = getPublishedInputPortTypes();
					char **names = getPublishedInputPortDetails();

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
					{
						char *newDetails = VuoRuntime_mergeEnumDetails(types[i], names[i]);
						if (newDetails)
							names[i] = newDetails;
						vuoInitMessageWithString(&messages[i], names[i]);
						if (newDetails)
							free(newDetails);
					}

					vuoControlReplySend(VuoControlReplyPublishedInputPortDetailsRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedOutputPortDetailsRetrieve:
				{
					int count = getPublishedOutputPortCount();
					char **names = getPublishedOutputPortDetails();

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
					vuoInitMessageWithString(&messages[i], names[i]);

					vuoControlReplySend(VuoControlReplyPublishedOutputPortDetailsRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedInputPortFireEvent:
				{
					char *name = vuoReceiveAndCopyString(ZMQControl, NULL);
					firePublishedInputPortEvent(name);
					free(name);

					vuoControlReplySend(VuoControlReplyPublishedInputPortFiredEvent,NULL,0);
					break;
				}
				case VuoControlRequestPublishedInputPortValueModify:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *valueAsString = vuoReceiveAndCopyString(ZMQControl, NULL);
					setPublishedInputPortValue(portIdentifier, valueAsString);
					free(portIdentifier);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyPublishedInputPortValueModified,NULL,0);
					break;
				}
				case VuoControlRequestPublishedInputPortValueRetrieve:
				{
					bool shouldUseInterprocessSerialization = vuoReceiveBool(ZMQControl, NULL);
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *valueAsString = getPublishedInputPortValue(portIdentifier, shouldUseInterprocessSerialization);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], valueAsString);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyPublishedInputPortValueRetrieved,messages,1);
					break;
				}
				case VuoControlRequestPublishedOutputPortValueRetrieve:
				{
					bool shouldUseInterprocessSerialization = vuoReceiveBool(ZMQControl, NULL);
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					char *valueAsString = getPublishedOutputPortValue(portIdentifier, shouldUseInterprocessSerialization);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], valueAsString);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyPublishedOutputPortValueRetrieved,messages,1);
					break;
				}
				case VuoControlRequestInputPortTelemetrySubscribe:
				case VuoControlRequestOutputPortTelemetrySubscribe:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					portsSendingDataTelemetry.insert(portIdentifier);
					bool isInput = (control == VuoControlRequestInputPortTelemetrySubscribe);
					char *summary = (isInput ? getInputPortSummary(portIdentifier) : getOutputPortSummary(portIdentifier));
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], summary);
					free(portIdentifier);
					free(summary);
					vuoControlReplySend(isInput ? VuoControlReplyInputPortTelemetrySubscribed : VuoControlReplyOutputPortTelemetrySubscribed,messages,1);
					break;
				}
				case VuoControlRequestInputPortTelemetryUnsubscribe:
				case VuoControlRequestOutputPortTelemetryUnsubscribe:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl, NULL);
					portsSendingDataTelemetry.erase(portIdentifier);
					free(portIdentifier);
					bool isInput = (control == VuoControlRequestInputPortTelemetryUnsubscribe);
					vuoControlReplySend(isInput ? VuoControlReplyInputPortTelemetryUnsubscribed : VuoControlReplyOutputPortTelemetryUnsubscribed,NULL,0);
					break;
				}
				case VuoControlRequestEventTelemetrySubscribe:
				{
					isSendingEventTelemetry = true;
					vuoControlReplySend(VuoControlReplyEventTelemetrySubscribed,NULL,0);
					break;
				}
				case VuoControlRequestEventTelemetryUnsubscribe:
				{
					isSendingEventTelemetry = false;
					vuoControlReplySend(VuoControlReplyEventTelemetryUnsubscribed,NULL,0);
					break;
				}
				case VuoControlRequestAllTelemetrySubscribe:
				{
					isSendingAllTelemetry = true;
					vuoControlReplySend(VuoControlReplyAllTelemetrySubscribed,NULL,0);
					break;
				}
				case VuoControlRequestAllTelemetryUnsubscribe:
				{
					isSendingAllTelemetry = false;
					vuoControlReplySend(VuoControlReplyAllTelemetryUnsubscribed,NULL,0);
					break;
				}
			}
		});
		dispatch_source_set_cancel_handler(controlTimer, ^{
			isStopped = true;
			VuoEventLoop_break();
			dispatch_semaphore_signal(controlCanceledSemaphore);
		});
		dispatch_resume(controlTimer);
	}


	// If the composition process should end if the runner's process ends, listen through the composition-runner
	// pipe to detect if the runner's process ends. If it does, stop the composition.
	if (! continueIfRunnerDies && runnerPipe >= 0)
	{
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		dispatch_async(queue, ^{
						   char buf[1];
						   int ret;
						   do {
							   ret = read(runnerPipe, &buf, 1);
						   } while (ret < 0);
						   hasZMQConnection = false;
						   vuoStopComposition();
					   });
	}
}


/**
 * Returns a string representation of the input port's current value.
 */
char * getInputPortString(char *portIdentifier, bool shouldUseInterprocessSerialization)
{
	return vuoGetPortValue(portIdentifier, shouldUseInterprocessSerialization ? 2 : 1);
}

/**
 * Returns a string representation of the output port's current value.
 */
char * getOutputPortString(char *portIdentifier, bool shouldUseInterprocessSerialization)
{
	return vuoGetPortValue(portIdentifier, shouldUseInterprocessSerialization ? 2 : 1);
}

/**
 * Returns a summary of the input port's current value.
 */
char * getInputPortSummary(char *portIdentifier)
{
	return vuoGetPortValue(portIdentifier, 0);
}

/**
 * Returns a summary of the output port's current value.
 */
char * getOutputPortSummary(char *portIdentifier)
{
	return vuoGetPortValue(portIdentifier, 0);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a node has started execution.
 */
void sendNodeExecutionStarted(char *nodeIdentifier)
{
	if (! (isSendingAllTelemetry || isSendingEventTelemetry))
		return;

	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], nodeIdentifier);

	vuoTelemetrySend(VuoTelemetryNodeExecutionStarted, messages, 1);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a node has finished execution.
 */
void sendNodeExecutionFinished(char *nodeIdentifier)
{
	if (! (isSendingAllTelemetry || isSendingEventTelemetry))
		return;

	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], nodeIdentifier);

	vuoTelemetrySend(VuoTelemetryNodeExecutionFinished, messages, 1);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an input port has received an event or data.
 */
void sendInputPortsUpdated(char *portIdentifier, bool receivedEvent, bool receivedData, char *portDataSummary)
{
	bool isSendingPortTelemetry = (portsSendingDataTelemetry.find(portIdentifier) != portsSendingDataTelemetry.end());
	if (! (isSendingAllTelemetry || isSendingEventTelemetry || isSendingPortTelemetry))
		return;

	zmq_msg_t messages[4];
	vuoInitMessageWithString(&messages[0], portIdentifier);
	vuoInitMessageWithBool(&messages[1], receivedEvent);
	vuoInitMessageWithBool(&messages[2], receivedData);
	vuoInitMessageWithString(&messages[3], (portDataSummary && (isSendingAllTelemetry || isSendingPortTelemetry)) ? portDataSummary : "");

	vuoTelemetrySend(VuoTelemetryInputPortsUpdated, messages, 4);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an output port has transmitted or fired an event.
 */
void sendOutputPortsUpdated(char *portIdentifier, bool sentData, char *portDataSummary)
{
	bool isSendingPortTelemetry = (portsSendingDataTelemetry.find(portIdentifier) != portsSendingDataTelemetry.end());
	if (! (isSendingAllTelemetry || isSendingEventTelemetry || isSendingPortTelemetry))
		return;

	zmq_msg_t messages[3];
	vuoInitMessageWithString(&messages[0], portIdentifier);
	vuoInitMessageWithBool(&messages[1], sentData);
	vuoInitMessageWithString(&messages[2], (portDataSummary && (isSendingAllTelemetry || isSendingPortTelemetry)) ? portDataSummary : "");

	vuoTelemetrySend(VuoTelemetryOutputPortsUpdated, messages, 3);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a published output port has transmitted an event.
 */
void sendPublishedOutputPortsUpdated(char *portIdentifier, bool sentData, char *portDataSummary)
{
	zmq_msg_t messages[3];
	vuoInitMessageWithString(&messages[0], portIdentifier);
	vuoInitMessageWithBool(&messages[1], sentData);
	vuoInitMessageWithString(&messages[2], (portDataSummary ? portDataSummary : ""));

	vuoTelemetrySend(VuoTelemetryPublishedOutputPortsUpdated, messages, 3);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a trigger port has dropped an event.
 */
void sendEventDropped(char *portIdentifier)
{
	bool isSendingPortTelemetry = (portsSendingDataTelemetry.find(portIdentifier) != portsSendingDataTelemetry.end());
	if (! (isSendingAllTelemetry || isSendingEventTelemetry || isSendingPortTelemetry))
		return;

	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], portIdentifier);

	vuoTelemetrySend(VuoTelemetryEventDropped, messages, 1);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an uncaught error has occurred.
 */
void sendError(const char *message)
{
	if (! hasZMQConnection)
		return;

	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], message);

	vuoTelemetrySend(VuoTelemetryError, messages, 1);
}

/**
 * Returns true if telemetry containing the port data summary should be sent for this port.
 */
bool vuoShouldSendPortDataTelemetry(const char *portIdentifier)
{
	bool isSendingPortTelemetry = (portsSendingDataTelemetry.find(portIdentifier) != portsSendingDataTelemetry.end());
	return (isSendingAllTelemetry || isSendingPortTelemetry);
}

/**
 * Serializes the variables controlling telemetry-sending to a JSON-formatted string.
 */
char * vuoSerializeTelemetryState(void)
{
	json_object *js = json_object_new_object();

	json_object *allObject = json_object_new_boolean(isSendingAllTelemetry);
	json_object_object_add(js, "isSendingAllTelemetry", allObject);

	json_object *eventObject = json_object_new_boolean(isSendingEventTelemetry);
	json_object_object_add(js, "isSendingEventTelemetry", eventObject);

	json_object *portsObject = json_object_new_array();
	for (set<string>::iterator i = portsSendingDataTelemetry.begin(); i != portsSendingDataTelemetry.end(); ++i)
	{
		string port = *i;
		json_object *portObject = json_object_new_string(port.c_str());
		json_object_array_add(portsObject, portObject);
	}
	json_object_object_add(js, "portsSendingDataTelemetry", portsObject);

	char *serialized = strdup(json_object_to_json_string_ext(js, JSON_C_TO_STRING_PLAIN));
	json_object_put(js);
	return serialized;
}

/**
 * Unserializes the variables controlling telemetry-sending from a JSON-formatted string.
 */
void vuoUnserializeTelemetryState(char *serialized)
{
	if (! serialized)
		return;

	json_object *js = json_tokener_parse(serialized);
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "isSendingAllTelemetry", &o))
		isSendingAllTelemetry = json_object_get_boolean(o);

	if (json_object_object_get_ex(js, "isSendingEventTelemetry", &o))
		isSendingEventTelemetry = json_object_get_boolean(o);

	if (json_object_object_get_ex(js, "portsSendingDataTelemetry", &o))
	{
		int portCount = json_object_array_length(o);
		for (int i = 0; i < portCount; ++i)
		{
			json_object *portObject = json_object_array_get_idx(o, i);
			string port = json_object_get_string(portObject);
			portsSendingDataTelemetry.insert(port);
		}
	}
}


static bool wasStopCompositionCalled = false;  ///< Prevents the composition from being finalized twice.

/**
 * This function is called when the composition receives a stop request (@ref VuoControlRequestCompositionStop),
 * or when the a node or library requests a clean shutdown (@ref VuoStopComposition).
 *
 * Stop requests are sent both when @ref VuoRunner::stop is called, and during live-coding reloads
 * (so, in the latter case, the composition process will resume after this function is called).
 *
 * @threadQueue{VuoControlQueue}
 */
void stopComposition(bool isBeingReplaced)
{
	dispatch_sync(VuoCompositionStopQueue, ^{

	// If we're stopping due to a user request, and we've already stopped, don't try to stop again.
	// (The 2-second timer in vuoStopComposition might ding while the previous call is still in progress.)
	if (wasStopCompositionCalled)
		return;
	wasStopCompositionCalled = true;

	dispatch_sync(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		if (hasBeenUnpaused)
		{
			if (!isPaused)
			{
				isPaused = true;
				vuoInstanceTriggerStop();
			}

			vuoInstanceFini();  // Called on a non-main thread to avoid deadlock with vuoTelemetrySend.
		}
	});

	if (!isBeingReplaced)
		dispatch_sync(VuoCompositionFiniCallbackQueue, ^{
						  for (VuoCompositionFiniCallbackListType::iterator i = VuoCompositionFiniCallbackList->begin(); i != VuoCompositionFiniCallbackList->end(); ++i)
							  (*i)();
					  });

	vuoCleanup();

	if (controlTimer)
		dispatch_source_cancel(controlTimer);
	else
	{
		isStopped = true;
		VuoEventLoop_break();
	}

	});
}

/**
 * Registers a callback to be invoked when the composition is shutting down,
 * after all nodes have been fini'd.
 *
 * Libraries should call the wrapper @ref VuoAddCompositionFiniCallback.
 */
void vuoAddCompositionFiniCallback(VuoCompositionFiniCallback fini)
{
	if (!VuoCompositionFiniCallbackQueue)
		return;

	dispatch_sync(VuoCompositionFiniCallbackQueue, ^{
					  VuoCompositionFiniCallbackList->push_back(fini);
				  });
}

/**
 * Nodes/libraries can call this function (via its wrapper, @ref VuoStopComposition)
 * to initiate a clean shutdown of the composition.
 *
 * It's also called if the VuoRunner dies, the composition is still running,
 * and VuoRunner has requested that the composition be stopped when it dies.
 */
void vuoStopComposition(void)
{
	isStopRequested = true;
	dispatch_async(VuoControlQueue, ^{
					   if (hasZMQConnection)
					   {
						   vuoTelemetrySend(VuoTelemetryStopRequested, NULL, 0);

						   // If we haven't received a response to VuoTelemetryStopRequested within 2 seconds, stop anyway.
						   waitForStopTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, VuoControlQueue);
						   dispatch_source_set_timer(waitForStopTimer, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2.), NSEC_PER_SEC * 2, NSEC_PER_SEC/10);
						   dispatch_source_set_event_handler(waitForStopTimer, ^{
							   stopComposition(false);
							   dispatch_source_cancel(waitForStopTimer);
						   });
						   dispatch_source_set_cancel_handler(waitForStopTimer, ^{
							   dispatch_semaphore_signal(waitForStopCanceledSemaphore);
						   });
						   dispatch_resume(waitForStopTimer);
					   }
					   else
					   {
						   stopComposition(false);
					   }
				   });

	// Break out of zmq_poll().
	if (ZMQSelfSend)
	{
		char z = 0;
		zmq_msg_t message;
		zmq_msg_init_size(&message, sizeof z);
		memcpy(zmq_msg_data(&message), &z, sizeof z);
		if (zmq_send(ZMQSelfSend, &message, 0) != 0)
			VUserLog("Couldn't break: %s (%d)", strerror(errno), errno);
		zmq_msg_close(&message);
	}
}


/**
 * Cleans up composition execution: closes the ZMQ sockets and dispatch source and queues.
 * Assumes the composition has received and replied to a @c VuoControlRequestCompositionStop message.
 */
void vuoFini(void)
{
	if (! hasZMQConnection)
		return;

	vuoMemoryBarrier();

	if (ZMQTelemetry)
	{
		// Cancel telemetryTimer, wait for it to stop, and clean up.
		dispatch_source_cancel(telemetryTimer);
		dispatch_semaphore_wait(telemetryCanceledSemaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(telemetryCanceledSemaphore);
		dispatch_release(telemetryTimer);
		dispatch_sync(VuoTelemetryQueue, ^{
						  zmq_close(ZMQTelemetry);
					  });
		dispatch_release(VuoTelemetryQueue);
	}

	if (ZMQControl)
	{
		// Wait for controlTimer to stop, and clean up.
		dispatch_semaphore_wait(controlCanceledSemaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(controlCanceledSemaphore);
		dispatch_release(controlTimer);
		dispatch_sync(VuoControlQueue, ^{
						  zmq_close(ZMQControl);
					  });
		dispatch_release(VuoControlQueue);
	}

	if (ZMQSelfSend)
	{
		zmq_close(ZMQSelfSend);
		ZMQSelfSend = NULL;
	}
	if (ZMQSelfReceive)
	{
		zmq_close(ZMQSelfReceive);
		ZMQSelfReceive = NULL;
	}

	if (waitForStopTimer)
	{
		dispatch_source_cancel(waitForStopTimer);
		dispatch_semaphore_wait(waitForStopCanceledSemaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(waitForStopCanceledSemaphore);
		dispatch_release(waitForStopTimer);
	}

	zmq_term(ZMQContext);
}

}  // extern "C"
