/**
 * @file
 * VuoRuntime implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <graphviz/gvc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#include <libgen.h> // for dirname()
#include <dirent.h>
#include <fcntl.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json/json.h>
#pragma clang diagnostic pop

#include "VuoRuntime.h"

bool hasZMQConnection = false;   ///< True if the @ref ZMQControl and @ref ZMQTelemetry sockets are connected to something.

dispatch_queue_t VuoControlQueue;	///< Dispatch queue for protecting access to the @c ZMQControl socket.
dispatch_queue_t VuoTelemetryQueue;	///< Dispatch queue for protecting access to the @c ZMQTelemetry socket.

void *ZMQContext;	///< The context used to initialize sockets.
void *ZMQControl;	///< The control socket. Use only on VuoControlQueue.
void *ZMQTelemetry = 0;	///< The telemetry socket. Use only on VuoControlQueue.

bool hasBeenUnpaused;  ///< True if node execution was unpaused initially, or if it has since been unpaused.
bool isPaused;  ///< True if node execution is currently paused.
bool isStopped; ///< True if composition execution has stopped.

dispatch_source_t telemetryTimer;  ///< Timer for sending telemetry messages.
dispatch_source_t controlTimer;  ///< Timer for receiving control messages.
dispatch_semaphore_t telemetryCanceledSemaphore;  ///< Signaled when telemetry events are no longer being processed.
dispatch_semaphore_t controlCanceledSemaphore;  ///< Signaled when control events are no longer being processed.


#include <graphviz/gvplugin.h>

extern gvplugin_library_t gvplugin_dot_layout_LTX_library; ///< Reference to the statically-built Graphviz Dot library.
extern gvplugin_library_t gvplugin_core_LTX_library; ///< Reference to the statically-built Graphviz core library.


/**
 * @threadQueue{VuoControlQueue}
 */
void vuoControlReplySend(enum VuoControlReply reply, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoControl",ZMQControl,reply,messages,messageCount,false);
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
		vuoSend("VuoTelemetry",ZMQTelemetry,type,messages,messageCount,true);
	});
}


//@{
/**
 * @internal
 * Defined in the composition's generated code.
 */
extern void setup(void);
extern void cleanup(void);
extern void nodeInstanceInit(void);
extern void nodeInstanceFini(void);
extern void nodeInstanceTriggerStart(void);
extern void nodeInstanceTriggerStop(void);
extern void setInputPortValue(char *portIdentifier, char *valueAsString, int shouldUpdateCallbacks);
extern void fireTriggerPortEvent(char *portIdentifier);
extern char * getInputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization);
extern char * getOutputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization);
extern char * getInputPortSummary(char *portIdentifier);
extern char * getOutputPortSummary(char *portIdentifier);
extern unsigned int getPublishedInputPortCount(void);
extern unsigned int getPublishedOutputPortCount(void);
extern char ** getPublishedInputPortNames(void);
extern char ** getPublishedOutputPortNames(void);
extern char ** getPublishedInputPortTypes(void);
extern char ** getPublishedOutputPortTypes(void);
extern int getPublishedInputPortConnectedIdentifierCount(char *name);
extern int getPublishedOutputPortConnectedIdentifierCount(char *name);
extern char ** getPublishedInputPortConnectedIdentifiers(char *name);
extern char ** getPublishedOutputPortConnectedIdentifiers(char *name);
extern void firePublishedInputPortEvent(char *name);
extern void VuoHeap_init();
extern void VuoHeap_fini();
//@}


/**
 * Parses command-line arguments, then calls @c vuoInitInProcess().
 */
void vuoInit(int argc, char **argv)
{
	char *controlURL = NULL;
	char *telemetryURL = NULL;
	bool _isPaused = false;
	bool doPrintHelp = false;
	bool doPrintLicenses = false;

	// parse commandline arguments
	{
		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"vuo-control", required_argument, NULL, 0},
			{"vuo-telemetry", required_argument, NULL, 0},
			{"vuo-pause", no_argument, NULL, 0},
			{"vuo-loader", required_argument, NULL, 0},
			{"vuo-licenses", no_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		while((getopt_long(argc, argv, "", options, &optionIndex)) != -1)
		{
			switch(optionIndex)
			{
				case 0:  // --help
					doPrintHelp = true;
					break;
				case 1:	// "vuo-control"
					controlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(controlURL, optarg);
					break;
				case 2:	// "vuo-telemetry"
					telemetryURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(telemetryURL, optarg);
					break;
				case 3: // "vuo-pause"
					_isPaused = true;
					break;
				case 4: // "vuo-loader" (ignored, but added here to avoid "unrecognized option" warning)
					break;
				case 5:  // --vuo-licenses
					doPrintLicenses = true;
					break;
			}
		}
	}

	if (doPrintHelp)
	{
		printf("Usage: %s [options]\n"
			   "Options:\n"
			   "  --help                                   Display this information.\n"
			   "  --vuo-licenses                           Display license information.\n"
			   "\n"
			   "  Remote control:\n"
			   "    --vuo-control=<transport://address>    Listen for control signals at the\n"
			   "                                           specified address. Supported\n"
			   "                                           protocols include 'ipc' and 'tcp'.\n"
			   "    --vuo-telemetry=<transport://address>  Send status updates to the specified\n"
			   "                                           address. Supported protocols include\n"
			   "                                           'ipc', 'tcp', 'pgm', and 'epgm'.\n"
			   "    --vuo-pause                            If specified, the composition starts\n"
			   "                                           paused (awaiting a vuo-control signal\n"
			   "                                           to begin execution).\n",
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

		const char *licensesPathFromExecutable = "/../Frameworks/Vuo.framework/Versions/" VUO_VERSION_STRING "/Licenses";
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

		exit(0);
	}

	VuoHeap_init();
	vuoInitInProcess(NULL, controlURL, telemetryURL, _isPaused);
}

/**
 * Sets up ZMQ control and telemetry sockets, then calls the generated function @c setup().
 * If the composition is not paused, also calls @c nodeInstanceInit() and @c nodeInstanceTriggerStart().
 */
void vuoInitInProcess(void *_ZMQContext, const char *controlURL, const char *telemetryURL, bool _isPaused)
{
//	fprintf(stderr, "\n\n# vuoInitInProcess()\n");
	if (controlURL && telemetryURL)
	{
		hasZMQConnection = true;
		ZMQContext = (_ZMQContext ? _ZMQContext : zmq_init(1));

		ZMQControl = zmq_socket(ZMQContext,ZMQ_REP);
		ZMQTelemetry = zmq_socket(ZMQContext,ZMQ_PUB);

		if(zmq_bind(ZMQControl,controlURL))
			fprintf(stderr, "VuoControl: bind failed (%s).\n", controlURL);

		if(zmq_bind(ZMQTelemetry,telemetryURL))
			fprintf(stderr, "VuoTelemetry: bind failed (%s).\n", telemetryURL);

		VuoTelemetryQueue = dispatch_queue_create("org.vuo.runtime.telemetry", NULL);
		VuoControlQueue = dispatch_queue_create("org.vuo.runtime.control", NULL);
	}

	hasBeenUnpaused = false;
	isPaused = _isPaused;
	isStopped = false;


	// set up composition
	{
		setup();

		if (! isPaused)
		{
			// Wait to call nodeInstanceInit() until the first time the composition enters an unpaused state.
			// If nodeInstanceInit() were called immediately when the composition is started in a paused state(),
			// then the vuo.event.fireOnStart node's fired event would be ignored.

			hasBeenUnpaused = true;
			__block bool initDone = false;
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   nodeInstanceInit();
							   nodeInstanceTriggerStart();
							   initDone = true;
						   });
			while (! initDone)
			{
				id pool = objc_msgSend((id)objc_getClass("NSAutoreleasePool"), sel_getUid("new"));
				CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.01,false);
				objc_msgSend(pool, sel_getUid("drain"));
			}
		}
	}


	// launch VuoTelemetryStats probe
	if (hasZMQConnection)
	{
		telemetryCanceledSemaphore = dispatch_semaphore_create(0);
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		telemetryTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,queue);
		dispatch_source_set_timer(telemetryTimer, dispatch_walltime(NULL, 0), NSEC_PER_SEC/2, NSEC_PER_SEC/100);
		dispatch_source_set_event_handler(telemetryTimer, ^{

			struct rusage r;
			if(getrusage(RUSAGE_SELF,&r))
				fprintf(stderr, "VuoTelemetry: failed to gather: %s\n", strerror(errno));

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
			};
			int itemCount = 1;
			long timeout = -1;  // wait indefinitely
			zmq_poll(items,itemCount,timeout);
			if(!(items[0].revents & ZMQ_POLLIN))
				return;

			enum VuoControlRequest control = (enum VuoControlRequest) vuoReceiveInt(ZMQControl);

			switch (control)
			{
				case VuoControlRequestCompositionStop:
				{
					int timeoutInSeconds = vuoReceiveInt(ZMQControl);

					if (timeoutInSeconds >= 0)
					{
						// If the composition is not stopped within the timeout, kill its process.
						dispatch_after(dispatch_time(DISPATCH_TIME_NOW, timeoutInSeconds * NSEC_PER_SEC),
									   dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							vuoControlReplySend(VuoControlReplyCompositionStopping,NULL,0);
							zmq_close(ZMQControl);  // wait until message fully sends
							kill(getpid(), SIGKILL);
						});
					}

					dispatch_sync(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
						if (hasBeenUnpaused)
						{
							if (!isPaused)
							{
								isPaused = true;
								nodeInstanceTriggerStop();
							}

							nodeInstanceFini();  // Called on a non-main thread to avoid deadlock with vuoTelemetrySend.
						}
					});
					cleanup();

					if (timeoutInSeconds >= 0)
					{
						VuoHeap_fini();
					}

					dispatch_source_cancel(controlTimer);

					vuoControlReplySend(VuoControlReplyCompositionStopping,NULL,0);

					break;
				}
				case VuoControlRequestCompositionPause:
				{
					isPaused = true;
					nodeInstanceTriggerStop();
					vuoControlReplySend(VuoControlReplyCompositionPaused,NULL,0);
					break;
				}
				case VuoControlRequestCompositionUnpause:
				{
					isPaused = false;

					if (! hasBeenUnpaused)
					{
						hasBeenUnpaused = true;
						nodeInstanceInit();
					}
					nodeInstanceTriggerStart();

					vuoControlReplySend(VuoControlReplyCompositionUnpaused,NULL,0);
					break;
				}
				case VuoControlRequestInputPortValueRetrieve:
				{
					bool shouldUseInterprocessSerialization = vuoReceiveBool(ZMQControl);
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl);
					char *valueAsString = getInputPortValue(portIdentifier, shouldUseInterprocessSerialization);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], valueAsString);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyInputPortValueRetrieved,messages,1);
					break;
				}
				case VuoControlRequestOutputPortValueRetrieve:
				{
					bool shouldUseInterprocessSerialization = vuoReceiveBool(ZMQControl);
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl);
					char *valueAsString = getOutputPortValue(portIdentifier, shouldUseInterprocessSerialization);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], valueAsString);
					free(valueAsString);
					vuoControlReplySend(VuoControlReplyOutputPortValueRetrieved,messages,1);
					break;
				}
				case VuoControlRequestInputPortSummaryRetrieve:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl);
					char *summary = getInputPortSummary(portIdentifier);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], summary);
					free(summary);
					vuoControlReplySend(VuoControlReplyInputPortSummaryRetrieved,messages,1);
					break;
				}
				case VuoControlRequestOutputPortSummaryRetrieve:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl);
					char *summary = getOutputPortSummary(portIdentifier);
					zmq_msg_t messages[1];
					vuoInitMessageWithString(&messages[0], summary);
					free(summary);
					vuoControlReplySend(VuoControlReplyOutputPortSummaryRetrieved,messages,1);
					break;
				}
				case VuoControlRequestTriggerPortFireEvent:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl);
					fireTriggerPortEvent(portIdentifier);
					free(portIdentifier);
					vuoControlReplySend(VuoControlReplyTriggerPortFiredEvent,NULL,0);
					break;
				}
				case VuoControlRequestInputPortValueModify:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQControl);
					char *valueAsString = vuoReceiveAndCopyString(ZMQControl);
					setInputPortValue(portIdentifier, valueAsString, true);
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
				case VuoControlRequestPublishedInputPortConnectedIdentifiersRetrieve:
				{
					char *name = vuoReceiveAndCopyString(ZMQControl);

					int count = getPublishedInputPortConnectedIdentifierCount(name);
					char **identifiers = getPublishedInputPortConnectedIdentifiers(name);

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
						vuoInitMessageWithString(&messages[i], identifiers[i]);

					vuoControlReplySend(VuoControlReplyPublishedInputPortConnectedIdentifiersRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedOutputPortConnectedIdentifiersRetrieve:
				{
					char *name = vuoReceiveAndCopyString(ZMQControl);

					int count = getPublishedOutputPortConnectedIdentifierCount(name);
					char **identifiers = getPublishedOutputPortConnectedIdentifiers(name);

					zmq_msg_t messages[count];
					for (int i = 0; i < count; ++i)
						vuoInitMessageWithString(&messages[i], identifiers[i]);

					vuoControlReplySend(VuoControlReplyPublishedOutputPortConnectedIdentifiersRetrieved,messages,count);
					break;
				}
				case VuoControlRequestPublishedInputPortFireEvent:
				{
					char *name = vuoReceiveAndCopyString(ZMQControl);
					firePublishedInputPortEvent(name);
					free(name);

					vuoControlReplySend(VuoControlReplyPublishedInputPortFiredEvent,NULL,0);
					break;
				}
				default:
				{
					fprintf(stderr, "composition: got unknown request %d\n", control);
					break;
				}
			}
		});
		dispatch_source_set_cancel_handler(controlTimer, ^{
			isStopped = true;
			dispatch_semaphore_signal(controlCanceledSemaphore);
		});
		dispatch_resume(controlTimer);
	}
}


/**
 * Constructs and sends a message on the telemetry socket, indicating that a node has started execution.
 */
void sendNodeExecutionStarted(char *nodeIdentifier)
{
	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], nodeIdentifier);

	vuoTelemetrySend(VuoTelemetryNodeExecutionStarted, messages, 1);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a node has finished execution.
 */
void sendNodeExecutionFinished(char *nodeIdentifier)
{
	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], nodeIdentifier);

	vuoTelemetrySend(VuoTelemetryNodeExecutionFinished, messages, 1);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an input port has received an event or data.
 */
void sendInputPortsUpdated(char *portIdentifier, bool receivedEvent, bool receivedData, char *portDataSummary)
{
	zmq_msg_t messages[4];
	vuoInitMessageWithString(&messages[0], portIdentifier);
	vuoInitMessageWithBool(&messages[1], receivedEvent);
	vuoInitMessageWithBool(&messages[2], receivedData);
	vuoInitMessageWithString(&messages[3], portDataSummary ? portDataSummary : "");

	vuoTelemetrySend(VuoTelemetryInputPortsUpdated, messages, 4);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an output port has transmitted or fired an event.
 */
void sendOutputPortsUpdated(char *portIdentifier, bool sentData, char *portDataSummary)
{
	zmq_msg_t messages[3];
	vuoInitMessageWithString(&messages[0], portIdentifier);
	vuoInitMessageWithBool(&messages[1], sentData);
	vuoInitMessageWithString(&messages[2], portDataSummary ? portDataSummary : "");

	vuoTelemetrySend(VuoTelemetryOutputPortsUpdated, messages, 3);
}

void vuoStopComposition();

/**
 * Constructs and sends a message on the telemetry socket, indicating that an uncaught error has occurred.
 */
void sendError(const char *message)
{
	/// @todo remove VRAM check after https://b33p.net/kosada/node/6909
	if (strcmp(message, "Out of video RAM.") == 0)
		vuoStopComposition();

	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], message);

	vuoTelemetrySend(VuoTelemetryError, messages, 1);
}

/**
 * Replaces '"' with '\"' and '\' with '\\' in a string.
 */
char * vuoTranscodeToGraphvizIdentifier(const char *originalString)
{
	size_t originalStringLen = strlen(originalString);
	size_t escapeCount = 0;
	for (size_t i = 0; i < originalStringLen; ++i)
		if (originalString[i] == '"' || originalString[i] == '\\')
			++escapeCount;

	size_t escapedStrlen = originalStringLen + escapeCount;
	char *escapedString = (char *)malloc(escapedStrlen + 1);
	for (size_t originalPos = 0, escapedPos = 0; originalPos < originalStringLen; ++originalPos, ++escapedPos)
	{
		if (originalString[originalPos] == '"' || originalString[originalPos] == '\\')
			escapedString[escapedPos++] = '\\';
		escapedString[escapedPos] = originalString[originalPos];
	}
	escapedString[escapedStrlen] = '\0';

	return escapedString;
}


const char *compositionDiff = NULL;  ///< A string containing a space-delimited list of nodes present in both the old and new composition, when replacing compositions for live coding.

/**
 * Returns true if the node is found in both the old and the new composition, when replacing compositions for live coding.
 *
 * This needs to be kept in sync with VuoCompilerComposition::diffAgainstOlderComposition().
 */
bool isNodeInBothCompositions(const char *nodeIdentifier)
{
	if (! compositionDiff)
		return false;

	json_object *diff = json_tokener_parse(compositionDiff);
	if (! diff)
		return false;

	bool isInChanges = false;
	int numChanges = json_object_array_length(diff);
	for (int i = 0; i < numChanges; ++i)
	{
		json_object *change = json_object_array_get_idx(diff, i);
		json_object *nodeIdentifierObj;
		if (json_object_object_get_ex(change, "add", &nodeIdentifierObj))
		{
			if (! strcmp(nodeIdentifier, json_object_get_string(nodeIdentifierObj)))
			{
				isInChanges = true;
				break;
			}
		}
		else if (json_object_object_get_ex(change, "remove", &nodeIdentifierObj))
		{
			if (! strcmp(nodeIdentifier, json_object_get_string(nodeIdentifierObj)))
			{
				isInChanges = true;
				break;
			}
		}
	}

	json_object_put(diff);

	return ! isInChanges;
}


GVC_t *graphvizContext = NULL;  ///< The context used when working with a Graphviz graph.

/**
 * Returns a Graphviz graph constructed from the given Graphviz-format string.
 */
graph_t * openGraphvizGraph(const char *graphString)
{
	// Use builtin Graphviz plugins, not demand-loaded plugins.
	lt_symlist_t lt_preloaded_symbols[] =
	{
		{ "gvplugin_dot_layout_LTX_library", &gvplugin_dot_layout_LTX_library},
		{ "gvplugin_core_LTX_library", &gvplugin_core_LTX_library},
		{ 0, 0}
	};
	bool demandLoading = false;
	graphvizContext = gvContextPlugins(lt_preloaded_symbols, demandLoading);

	graph_t *graph = agmemread((char *)graphString);
	agraphattr(graph, (char *)"rankdir", (char *)"LR");
	agnodeattr(graph, (char *)"shape", (char *)"Mrecord");
	gvLayout(graphvizContext, graph, "dot");

	return graph;
}

/**
 * Cleans up a Graphviz graph when it is no longer in use.
 */
void closeGraphvizGraph(graph_t * graph)
{
	gvFreeLayout(graphvizContext, graph);
	agclose(graph);
	gvFreeContext(graphvizContext);

	graphvizContext = NULL;
}

/**
 * Returns the constant value of the input port or instance data listed in the Graphviz graph,
 * or null if it is not listed.
 */
const char * getConstantValueFromGraphviz(graph_t * graph, const char *node, const char *port)
{
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		if (strcmp(n->name, node))
			continue;

		field_t *nodeInfo = (field_t *)ND_shape_info(n);
		int numNodeInfoFields = nodeInfo->n_flds;

		for (int i = 0; i < numNodeInfoFields; i++)
		{
			field_t *nodeInfoField = nodeInfo->fld[i];
			if (nodeInfoField->id == NULL || strcmp(nodeInfoField->id, port))
				continue;

			char portInDotInitializer[strlen("_")+strlen(port)+1];
			sprintf(portInDotInitializer, "_%s", port);

			char *portConstant = agget(n, portInDotInitializer);
			if (!portConstant)
				return NULL;

			size_t portConstantLen = strlen(portConstant);
			char *portConstantUnescaped = (char *)malloc(portConstantLen+1);
			int k = 0;
			for (int j = 0; j < portConstantLen; ++j)
			{
				if (j < portConstantLen-1 && portConstant[j] == '\\' && portConstant[j+1] == '\\')
					++j;
				portConstantUnescaped[k++] = portConstant[j];
			}
			portConstantUnescaped[k] = 0;

			return portConstantUnescaped;
		}
	}

	return NULL;
}


/**
 * Cleanly stops the composition.
 */
void vuoStopComposition(void)
{
	if (hasZMQConnection)
	{
		vuoTelemetrySend(VuoTelemetryStopRequested, NULL, 0);
	}
	else
	{
		dispatch_sync(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
						  if (hasBeenUnpaused)
						  {
							  if (!isPaused)
							  {
								  isPaused = true;
								  nodeInstanceTriggerStop();
							  }

							  nodeInstanceFini();
						  }
					  });
		cleanup();

		isStopped = true;
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

	// Cancel telemetryTimer, wait for it to stop, and clean up.
	dispatch_source_cancel(telemetryTimer);
	dispatch_semaphore_wait(telemetryCanceledSemaphore, DISPATCH_TIME_FOREVER);
	dispatch_release(telemetryCanceledSemaphore);
	dispatch_release(telemetryTimer);
	dispatch_sync(VuoTelemetryQueue, ^{
		zmq_close(ZMQTelemetry);
	});
	dispatch_release(VuoTelemetryQueue);

	// Wait for controlTimer to stop, and clean up.
	dispatch_semaphore_wait(controlCanceledSemaphore, DISPATCH_TIME_FOREVER);
	dispatch_release(controlCanceledSemaphore);
	dispatch_release(controlTimer);
	dispatch_sync(VuoControlQueue, ^{
		zmq_close(ZMQControl);
	});
	dispatch_release(VuoControlQueue);

	zmq_term(ZMQContext);
}
