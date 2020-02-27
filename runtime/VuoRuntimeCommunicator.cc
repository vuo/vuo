/**
 * @file
 * VuoRuntimeCommunicator implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRuntimeCommunicator.hh"

#include <dlfcn.h>
#include <sstream>
#include "VuoEventLoop.h"
#include "VuoException.hh"
#include "VuoHeap.h"
#include "VuoNodeRegistry.hh"
#include "VuoRuntimePersistentState.hh"
#include "VuoRuntimeState.hh"

/**
 * Constructor. Does not take ownership of @a persistentState.
 */
VuoRuntimeCommunicator::VuoRuntimeCommunicator(VuoRuntimePersistentState *persistentState)
{
	this->persistentState = persistentState;

	_hasZmqConnection = false;

	zmqContext = NULL;
	zmqControl = NULL;
	zmqSelfReceive = NULL;
	zmqSelfSend = NULL;
	zmqTelemetry = NULL;

	controlQueue = dispatch_queue_create("org.vuo.runtime.control", NULL);
	telemetryQueue = dispatch_queue_create("org.vuo.runtime.telemetry", NULL);
	controlTimer = NULL;
	telemetryTimer = NULL;
	controlCanceled = dispatch_semaphore_create(0);
	telemetryCanceled = dispatch_semaphore_create(0);

	runnerPipe = -1;

	vuoInstanceInit = NULL;
	vuoInstanceTriggerStart = NULL;
	vuoInstanceTriggerStop = NULL;
	vuoSetInputPortValue = NULL;
	getPublishedInputPortCount = NULL;
	getPublishedOutputPortCount = NULL;
	getPublishedInputPortNames = NULL;
	getPublishedOutputPortNames = NULL;
	getPublishedInputPortTypes = NULL;
	getPublishedOutputPortTypes = NULL;
	getPublishedInputPortDetails = NULL;
	getPublishedOutputPortDetails = NULL;
	firePublishedInputPortEvent = NULL;
	setPublishedInputPortValue = NULL;
	getPublishedInputPortValue = NULL;
	getPublishedOutputPortValue = NULL;
}

/**
 * Destructor.
 */
VuoRuntimeCommunicator::~VuoRuntimeCommunicator(void)
{
	dispatch_release(controlQueue);
	dispatch_release(telemetryQueue);
	dispatch_release(controlCanceled);
	dispatch_release(telemetryCanceled);
}

/**
 * Updates references to symbols defined in the composition's generated code.
 *
 * @throw VuoException One of the symbols was not found in the composition binary.
 */
void VuoRuntimeCommunicator::updateCompositionSymbols(void *compositionBinaryHandle)
{
	ostringstream errorMessage;

	vuoInstanceInit = (vuoInstanceInitType) dlsym(compositionBinaryHandle, "vuoInstanceInit");
	if (! vuoInstanceInit)
	{
		errorMessage << "The composition couldn't be started because its vuoInstanceInit() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	vuoInstanceTriggerStart = (vuoInstanceTriggerStartType) dlsym(compositionBinaryHandle, "vuoInstanceTriggerStart");
	if (! vuoInstanceTriggerStart)
	{
		errorMessage << "The composition couldn't be started because its vuoInstanceTriggerStart() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	vuoInstanceTriggerStop = (vuoInstanceTriggerStopType) dlsym(compositionBinaryHandle, "vuoInstanceTriggerStop");
	if (! vuoInstanceTriggerStop)
	{
		errorMessage << "The composition couldn't be started because its vuoInstanceTriggerStop() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	vuoSetInputPortValue = (vuoSetInputPortValueType) dlsym(compositionBinaryHandle, "vuoSetInputPortValue");
	if (! vuoSetInputPortValue)
	{
		errorMessage << "The composition couldn't be started because its vuoSetInputPortValue() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedInputPortCount = (getPublishedInputPortCountType) dlsym(compositionBinaryHandle, "getPublishedInputPortCount");
	if (! getPublishedInputPortCount)
	{
		errorMessage << "The composition couldn't be started because its getPublishedInputPortCount() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedOutputPortCount = (getPublishedOutputPortCountType) dlsym(compositionBinaryHandle, "getPublishedOutputPortCount");
	if (! getPublishedOutputPortCount)
	{
		errorMessage << "The composition couldn't be started because its getPublishedOutputPortCount() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedInputPortNames = (getPublishedInputPortNamesType) dlsym(compositionBinaryHandle, "getPublishedInputPortNames");
	if (! getPublishedInputPortNames)
	{
		errorMessage << "The composition couldn't be started because its getPublishedInputPortNames() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedOutputPortNames = (getPublishedOutputPortNamesType) dlsym(compositionBinaryHandle, "getPublishedOutputPortNames");
	if (! getPublishedOutputPortNames)
	{
		errorMessage << "The composition couldn't be started because its getPublishedOutputPortNames() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedInputPortTypes = (getPublishedInputPortTypesType) dlsym(compositionBinaryHandle, "getPublishedInputPortTypes");
	if (! getPublishedInputPortTypes)
	{
		errorMessage << "The composition couldn't be started because its getPublishedInputPortTypes() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedOutputPortTypes = (getPublishedOutputPortTypesType) dlsym(compositionBinaryHandle, "getPublishedOutputPortTypes");
	if (! getPublishedOutputPortTypes)
	{
		errorMessage << "The composition couldn't be started because its getPublishedOutputPortTypes() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedInputPortDetails = (getPublishedInputPortDetailsType) dlsym(compositionBinaryHandle, "getPublishedInputPortDetails");
	if (! getPublishedInputPortDetails)
	{
		errorMessage << "The composition couldn't be started because its getPublishedInputPortDetails() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedOutputPortDetails = (getPublishedOutputPortDetailsType) dlsym(compositionBinaryHandle, "getPublishedOutputPortDetails");
	if (! getPublishedOutputPortDetails)
	{
		errorMessage << "The composition couldn't be started because its getPublishedOutputPortDetails() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	firePublishedInputPortEvent = (firePublishedInputPortEventType) dlsym(compositionBinaryHandle, "firePublishedInputPortEvent");
	if (! firePublishedInputPortEvent)
	{
		errorMessage << "The composition couldn't be started because its firePublishedInputPortEvent() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	setPublishedInputPortValue = (setPublishedInputPortValueType) dlsym(compositionBinaryHandle, "setPublishedInputPortValue");
	if (! setPublishedInputPortValue)
	{
		errorMessage << "The composition couldn't be started because its setPublishedInputPortValue() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedInputPortValue = (getPublishedInputPortValueType) dlsym(compositionBinaryHandle, "getPublishedInputPortValue");
	if (! getPublishedInputPortValue)
	{
		errorMessage << "The composition couldn't be started because its getPublishedInputPortValue() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}

	getPublishedOutputPortValue = (getPublishedOutputPortValueType) dlsym(compositionBinaryHandle, "getPublishedOutputPortValue");
	if (! getPublishedOutputPortValue)
	{
		errorMessage << "The composition couldn't be started because its getPublishedOutputPortValue() function couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}
}

/**
 * Sets up the ZMQ connection and pipe between this class and the runner.
 *
 * @throw VuoException @a controlURL and @a telemetryURL were provided, but the ZMQ connections couldn't be established.
 */
void VuoRuntimeCommunicator::openConnection(void *_zmqContext, const char *controlURL, const char *telemetryURL, int runnerPipe)
{
	this->runnerPipe = runnerPipe;

	if (controlURL && telemetryURL)
	{
		_hasZmqConnection = true;
		zmqContext = (_zmqContext ? _zmqContext : zmq_init(1));

		zmqControl = zmq_socket(zmqContext,ZMQ_REP);
		zmqTelemetry = zmq_socket(zmqContext,ZMQ_PUB);

		bool error = false;
		ostringstream errorMessage;

		if(zmq_bind(zmqControl,controlURL))
		{
			errorMessage << "The composition couldn't start because it couldn't establish communication to be controlled by the runner : " << zmq_strerror(errno) << endl;
			error = true;
		}

		zmqSelfReceive = zmq_socket(zmqContext, ZMQ_PAIR);
		if (zmq_bind(zmqSelfReceive, "inproc://vuo-runtime-self") != 0)
		{
			errorMessage << "Couldn't bind self-receive socket: " << zmq_strerror(errno) << " " << errno << endl;
			error = true;
		}

		zmqSelfSend = zmq_socket(zmqContext, ZMQ_PAIR);
		if (zmq_connect(zmqSelfSend, "inproc://vuo-runtime-self") != 0)
		{
			errorMessage << "Couldn't connect self-send socket: " << zmq_strerror(errno) << " " << errno << endl;
			error = true;
		}

		if(zmq_bind(zmqTelemetry,telemetryURL))
		{
			errorMessage << "The composition couldn't start because it couldn't establish communication to be listened to by the runner : " << zmq_strerror(errno) << endl;
			error = true;
		}

		if (error)
		{
			zmq_close(zmqControl);
			zmqControl = NULL;
			zmq_close(zmqSelfSend);
			zmqSelfSend = NULL;
			zmq_close(zmqSelfReceive);
			zmqSelfReceive = NULL;
			zmq_close(zmqTelemetry);
			zmqTelemetry = NULL;
			throw VuoException(errorMessage.str());
		}
	}
}

/**
 * Terminates the ZMQ connection between this class and the composition.
 */
void VuoRuntimeCommunicator::closeConnection(void)
{
	if (! _hasZmqConnection)
		return;

	zmq_term(zmqContext);
	zmqContext = NULL;

	_hasZmqConnection = false;
}

/**
 * Returns true if the runtime has a ZMQ connection to the runner.
 */
bool VuoRuntimeCommunicator::hasZmqConnection(void)
{
	return _hasZmqConnection;
}

/**
 * Returns the dispatch queue on which this object sends control reply messages.
 */
dispatch_queue_t VuoRuntimeCommunicator::getControlQueue(void)
{
	return controlQueue;
}

/**
 * @threadQueue{VuoControlQueue}
 */
void VuoRuntimeCommunicator::sendControlReply(enum VuoControlReply reply, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoControl",zmqControl,reply,messages,messageCount,false,NULL);
}

/**
 * @threadAny
 */
void VuoRuntimeCommunicator::sendTelemetry(enum VuoTelemetry type, zmq_msg_t *messages, unsigned int messageCount)
{
	/// @todo https://b33p.net/kosada/node/5567
	if (!zmqTelemetry)
		return;

	dispatch_sync(telemetryQueue, ^{
		vuoMemoryBarrier();
		vuoSend("VuoTelemetry",zmqTelemetry,type,messages,messageCount,true,NULL);
	});
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a node has started execution.
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRuntimeCommunicator::sendNodeExecutionStarted(const char *compositionIdentifier, const char *nodeIdentifier)
{
	if (! (isSubscribedToAllTelemetry(compositionIdentifier) || isSubscribedToEventTelemetry(compositionIdentifier)))
		return;

	zmq_msg_t messages[2];
	vuoInitMessageWithString(&messages[0], compositionIdentifier);
	vuoInitMessageWithString(&messages[1], nodeIdentifier);

	sendTelemetry(VuoTelemetryNodeExecutionStarted, messages, 2);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a node has finished execution.
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRuntimeCommunicator::sendNodeExecutionFinished(const char *compositionIdentifier, const char *nodeIdentifier)
{
	if (! (isSubscribedToAllTelemetry(compositionIdentifier) || isSubscribedToEventTelemetry(compositionIdentifier)))
		return;

	zmq_msg_t messages[2];
	vuoInitMessageWithString(&messages[0], compositionIdentifier);
	vuoInitMessageWithString(&messages[1], nodeIdentifier);

	sendTelemetry(VuoTelemetryNodeExecutionFinished, messages, 2);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an input port has received an event and/or data.
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRuntimeCommunicator::sendInputPortsUpdated(const char *compositionIdentifier, const char *portIdentifier, bool receivedEvent, bool receivedData, const char *portDataSummary)
{
	bool isSendingAllTelemetry = isSubscribedToAllTelemetry(compositionIdentifier);
	bool isSendingPortTelemetry = isSubscribedToPortDataTelemetry(compositionIdentifier, portIdentifier);
	if (! (isSendingAllTelemetry || isSubscribedToEventTelemetry(compositionIdentifier) || isSendingPortTelemetry))
		return;

	zmq_msg_t messages[5];
	vuoInitMessageWithString(&messages[0], compositionIdentifier);
	vuoInitMessageWithString(&messages[1], portIdentifier);
	vuoInitMessageWithBool(&messages[2], receivedEvent);
	vuoInitMessageWithBool(&messages[3], receivedData);
	vuoInitMessageWithString(&messages[4], (portDataSummary && (isSendingAllTelemetry || isSendingPortTelemetry)) ? portDataSummary : "");

	sendTelemetry(VuoTelemetryInputPortsUpdated, messages, 5);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an output port has transmitted/fired an event and/or data.
 *
 * @version200Changed{Added `compositionIdentifier`, `sentEvent` arguments.}
 */
void VuoRuntimeCommunicator::sendOutputPortsUpdated(const char *compositionIdentifier, const char *portIdentifier, bool sentEvent, bool sentData, const char *portDataSummary)
{
	bool isSendingAllTelemetry = isSubscribedToAllTelemetry(compositionIdentifier);
	bool isSendingPortTelemetry = isSubscribedToPortDataTelemetry(compositionIdentifier, portIdentifier);
	if (! (isSendingAllTelemetry || isSubscribedToEventTelemetry(compositionIdentifier) || isSendingPortTelemetry))
		return;

	zmq_msg_t messages[5];
	vuoInitMessageWithString(&messages[0], compositionIdentifier);
	vuoInitMessageWithString(&messages[1], portIdentifier);
	vuoInitMessageWithBool(&messages[2], sentEvent);
	vuoInitMessageWithBool(&messages[3], sentData);
	vuoInitMessageWithString(&messages[4], (portDataSummary && (isSendingAllTelemetry || isSendingPortTelemetry)) ? portDataSummary : "");

	sendTelemetry(VuoTelemetryOutputPortsUpdated, messages, 5);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a published output port has transmitted an event.
 */
void VuoRuntimeCommunicator::sendPublishedOutputPortsUpdated(const char *portIdentifier, bool sentData, const char *portDataSummary)
{
	zmq_msg_t messages[3];
	vuoInitMessageWithString(&messages[0], portIdentifier);
	vuoInitMessageWithBool(&messages[1], sentData);
	vuoInitMessageWithString(&messages[2], (portDataSummary ? portDataSummary : ""));

	sendTelemetry(VuoTelemetryPublishedOutputPortsUpdated, messages, 3);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an event has finished propagating through the top-level composition.
 *
 * @version200New
 */
void VuoRuntimeCommunicator::sendEventFinished(unsigned long eventId, NodeContext *compositionContext)
{
	if (! vuoFinishedExecutingEvent(compositionContext, eventId))
		return;

	sendTelemetry(VuoTelemetryEventFinished, NULL, 0);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that a trigger port has dropped an event.
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRuntimeCommunicator::sendEventDropped(const char *compositionIdentifier, const char *portIdentifier)
{
	bool isSendingPortTelemetry = isSubscribedToPortDataTelemetry(compositionIdentifier, portIdentifier);
	if (! (isSubscribedToAllTelemetry(compositionIdentifier) || isSubscribedToEventTelemetry(compositionIdentifier) || isSendingPortTelemetry))
		return;

	zmq_msg_t messages[2];
	vuoInitMessageWithString(&messages[0], compositionIdentifier);
	vuoInitMessageWithString(&messages[1], portIdentifier);

	sendTelemetry(VuoTelemetryEventDropped, messages, 2);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that an uncaught error has occurred.
 */
void VuoRuntimeCommunicator::sendError(const char *message)
{
	if (! _hasZmqConnection)
		return;

	zmq_msg_t messages[1];
	vuoInitMessageWithString(&messages[0], message);

	sendTelemetry(VuoTelemetryError, messages, 1);
}

/**
 * Constructs and sends a message on the telemetry socket, indicating that the composition would like to stop itself.
 */
void VuoRuntimeCommunicator::sendStopRequested(void)
{
	sendTelemetry(VuoTelemetryStopRequested, NULL, 0);
}

/**
 * Constructs and sends a message on the control socket, confirming that the composition is shutting down,
 * and closes the control socket.
 */
void VuoRuntimeCommunicator::sendCompositionStoppingAndCloseControl(void)
{
	sendControlReply(VuoControlReplyCompositionStopping, NULL, 0);
	zmq_close(zmqControl);  // wait until message fully sends
	zmqControl = NULL;
}

/**
 * Adds the port to the list of those subscribed to telemetry.
 */
void VuoRuntimeCommunicator::subscribeToPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifer)
{
	portsSendingDataTelemetry[compositionIdentifier].insert(portIdentifer);
}

/**
 * Removes the port to the list of those subscribed to telemetry.
 */
void VuoRuntimeCommunicator::unsubscribeFromPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifer)
{
	map<string, set<string> >::iterator iter1 = portsSendingDataTelemetry.find(compositionIdentifier);
	if (iter1 != portsSendingDataTelemetry.end())
	{
		set<string>::iterator iter2 = iter1->second.find(portIdentifer);
		if (iter2 != iter1->second.end())
		{
			iter1->second.erase(iter2);
			if (iter1->second.empty())
				portsSendingDataTelemetry.erase(iter1);
		}
	}
}

/**
 * Returns true if this port is subscribed to telemetry.
 */
bool VuoRuntimeCommunicator::isSubscribedToPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifer)
{
	map<string, set<string> >::iterator iter = portsSendingDataTelemetry.find(compositionIdentifier);
	if (iter != portsSendingDataTelemetry.end())
		return (iter->second.find(portIdentifer) != iter->second.end());

	return false;
}

/**
 * Adds the (sub)composition instance to the list of those subscribed to event telemetry.
 */
void VuoRuntimeCommunicator::subscribeToEventTelemetry(const char *compositionIdentifier)
{
	compositionsSendingEventTelemetry.insert(compositionIdentifier);
}

/**
 * Removes the (sub)composition instance from the list of those subscribed to event telemetry.
 */
void VuoRuntimeCommunicator::unsubscribeFromEventTelemetry(const char *compositionIdentifier)
{
	set<string>::iterator iter = compositionsSendingEventTelemetry.find(compositionIdentifier);
	if (iter != compositionsSendingEventTelemetry.end())
		compositionsSendingEventTelemetry.erase(iter);
}

/**
 * Returns true if the (sub)composition instance is subscribed to event telemetry.
 */
bool VuoRuntimeCommunicator::isSubscribedToEventTelemetry(const char *compositionIdentifier)
{
	set<string>::iterator iter = compositionsSendingEventTelemetry.find(compositionIdentifier);
	return (iter != compositionsSendingEventTelemetry.end());
}

/**
 * Adds the (sub)composition instance to the list of those subscribed to all telemetry.
 */
void VuoRuntimeCommunicator::subscribeToAllTelemetry(const char *compositionIdentifier)
{
	compositionsSendingAllTelemetry.insert(compositionIdentifier);
}

/**
 * Removes the (sub)composition instance from the list of those subscribed to all telemetry.
 */
void VuoRuntimeCommunicator::unsubscribeFromAllTelemetry(const char *compositionIdentifier)
{
	set<string>::iterator iter = compositionsSendingAllTelemetry.find(compositionIdentifier);
	if (iter != compositionsSendingAllTelemetry.end())
		compositionsSendingAllTelemetry.erase(iter);
}

/**
 * Returns true if the (sub)composition instance is subscribed to all telemetry.
 */
bool VuoRuntimeCommunicator::isSubscribedToAllTelemetry(const char *compositionIdentifier)
{
	set<string>::iterator iter = compositionsSendingAllTelemetry.find(compositionIdentifier);
	return (iter != compositionsSendingAllTelemetry.end());
}

/**
 * Returns true if telemetry containing the port data summary should be sent for this port.
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
bool VuoRuntimeCommunicator::shouldSendPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifier)
{
	return (isSubscribedToAllTelemetry(compositionIdentifier) || isSubscribedToPortDataTelemetry(compositionIdentifier, portIdentifier));
}

/**
 * If `type` is an enum, merges the allowed enum values into `details` and returns it (caller responsible for freeing).
 *
 * Otherwise, returns NULL.
 */
char * VuoRuntimeCommunicator::mergeEnumDetails(string type, const char *details)
{
	string allowedValuesFunctionName = type + "_getAllowedValues";
	typedef void *(*allowedValuesFunctionType)(void);
	allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_SELF, allowedValuesFunctionName.c_str());
	if (!allowedValuesFunction)
		return NULL;

	string getJsonFunctionName = type + "_getJson";
	typedef json_object *(*getJsonFunctionType)(int64_t);
	getJsonFunctionType getJsonFunction = (getJsonFunctionType)dlsym(RTLD_SELF, getJsonFunctionName.c_str());
	if (!getJsonFunction)
		return NULL;

	string summaryFunctionName = type + "_getSummary";
	typedef char *(*summaryFunctionType)(int64_t);
	summaryFunctionType summaryFunction = (summaryFunctionType)dlsym(RTLD_SELF, summaryFunctionName.c_str());
	if (!summaryFunction)
		return NULL;

	string listCountFunctionName = "VuoListGetCount_" + type;
	typedef unsigned long (*listCountFunctionType)(void *);
	listCountFunctionType listCountFunction = (listCountFunctionType)dlsym(RTLD_SELF, listCountFunctionName.c_str());
	if (!listCountFunction)
		return NULL;

	string listValueFunctionName = "VuoListGetValue_" + type;
	typedef int64_t (*listValueFunctionType)(void *, unsigned long);
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
		int64_t value = listValueFunction(allowedValues, i);
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

	if (json_object_array_length(menuItems))
		json_object_object_add(detailsJson, "menuItems", menuItems);

	char *newDetails = strdup(json_object_to_json_string(detailsJson));
	json_object_put(detailsJson);

	return newDetails;
}

void VuoRuntimeCommunicator::sendHeartbeat(bool blocking)
{
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

	if (blocking)
		// When called with blocking=true, we're already on telemetryQueue.
		vuoSend("VuoTelemetry", zmqTelemetry, VuoTelemetryStats, messages, 2, false, NULL);
	else
		sendTelemetry(VuoTelemetryStats, messages, 2);
}

/**
 * Starts a timer to periodically send heartbeat telemetry to the runner.
 */
void VuoRuntimeCommunicator::startSendingHeartbeat(void)
{
	if (! zmqTelemetry)
		return;

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
	telemetryTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, queue);
	dispatch_source_set_timer(telemetryTimer, dispatch_walltime(NULL, 0), NSEC_PER_SEC/1000, NSEC_PER_SEC/1000);

	dispatch_source_set_event_handler(telemetryTimer, ^{
		sendHeartbeat();
	});

	dispatch_source_set_cancel_handler(telemetryTimer, ^{
		dispatch_semaphore_signal(telemetryCanceled);
	});

	dispatch_resume(telemetryTimer);
}

/**
 * Stops the telemetry timer and cleans up.
 */
void VuoRuntimeCommunicator::stopSendingAndCleanUpHeartbeat(void)
{
	if (! zmqTelemetry)
		return;

	dispatch_source_cancel(telemetryTimer);
	dispatch_semaphore_wait(telemetryCanceled, DISPATCH_TIME_FOREVER);
	dispatch_sync(telemetryQueue, ^{
					  // zmq_close calls POSIX close(), whose documentation says "queued data are discarded".
					  // Since telemetry uses non-blocking sends, issue one final _blocking_ send
					  // to ensure that the queue is drained before we close.
					  sendHeartbeat(true);

					  zmq_close(zmqTelemetry);
					  zmqTelemetry = NULL;
				  });

	dispatch_release(telemetryTimer);
	telemetryTimer = NULL;
}

/**
 * Starts a timer to periodically listen for and respond to control messages from the runner.
 */
void VuoRuntimeCommunicator::startListeningForControl(void)
{
	if (! zmqControl)
		return;

	controlTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, controlQueue);
	dispatch_source_set_timer(controlTimer, dispatch_walltime(NULL,0), NSEC_PER_SEC/1000, NSEC_PER_SEC/1000);

	dispatch_source_set_event_handler(controlTimer, ^{

		vuoMemoryBarrier();

		zmq_pollitem_t items[]=
		{
			{zmqControl,0,ZMQ_POLLIN,0},
			{zmqSelfReceive,0,ZMQ_POLLIN,0},
		};
		int itemCount = 2;
		long timeout = -1;  // Wait forever (VuoStopComposition will send a message ZMQSelfReceive when it's time to stop).
		zmq_poll(items,itemCount,timeout);
		if(!(items[0].revents & ZMQ_POLLIN))
			return;

		enum VuoControlRequest control = (enum VuoControlRequest) vuoReceiveInt(zmqControl, NULL);
		VuoCompositionState compositionState = { (void *)persistentState->runtimeState, "" };

		switch (control)
		{
			case VuoControlRequestSlowHeartbeat:
			{
				dispatch_source_set_timer(telemetryTimer, dispatch_walltime(NULL, 0), NSEC_PER_SEC/2, NSEC_PER_SEC/100);

				sendControlReply(VuoControlReplyHeartbeatSlowed,NULL,0);
				break;
			}
			case VuoControlRequestCompositionStop:
			{
				int timeoutInSeconds = vuoReceiveInt(zmqControl, NULL);
				bool isBeingReplaced = vuoReceiveBool(zmqControl, NULL);
				bool isLastEverInProcess = vuoReceiveBool(zmqControl, NULL);

				persistentState->runtimeState->stopCompositionAsOrderedByRunner(isBeingReplaced, timeoutInSeconds, isLastEverInProcess);

				sendControlReply(VuoControlReplyCompositionStopping,NULL,0);
				break;
			}
			case VuoControlRequestCompositionPause:
			{
				persistentState->runtimeState->pauseComposition();

				sendControlReply(VuoControlReplyCompositionPaused,NULL,0);
				break;
			}
			case VuoControlRequestCompositionUnpause:
			{
				persistentState->runtimeState->unpauseComposition();

				sendControlReply(VuoControlReplyCompositionUnpaused,NULL,0);
				break;
			}
			case VuoControlRequestInputPortValueRetrieve:
			case VuoControlRequestOutputPortValueRetrieve:
			{
				bool shouldUseInterprocessSerialization = vuoReceiveBool(zmqControl, NULL);
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				char *valueAsString = persistentState->nodeRegistry->getPortValue(&compositionState, portIdentifier, shouldUseInterprocessSerialization);

				zmq_msg_t messages[1];
				vuoInitMessageWithString(&messages[0], valueAsString);
				sendControlReply(control == VuoControlRequestInputPortValueRetrieve ?
									VuoControlReplyInputPortValueRetrieved : VuoControlReplyOutputPortValueRetrieved,
									messages,1);

				free(compositionIdentifier);
				free(portIdentifier);
				free(valueAsString);
				break;
			}
			case VuoControlRequestInputPortSummaryRetrieve:
			case VuoControlRequestOutputPortSummaryRetrieve:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				char *summary = persistentState->nodeRegistry->getPortSummary(&compositionState, portIdentifier);

				zmq_msg_t messages[1];
				vuoInitMessageWithString(&messages[0], summary);
				sendControlReply(control == VuoControlRequestInputPortSummaryRetrieve ?
									 VuoControlReplyInputPortSummaryRetrieved : VuoControlReplyOutputPortSummaryRetrieved,
									 messages,1);

				free(compositionIdentifier);
				free(portIdentifier);
				free(summary);
				break;
			}
			case VuoControlRequestTriggerPortFireEvent:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				persistentState->nodeRegistry->fireTriggerPortEvent(&compositionState, portIdentifier);

				sendControlReply(VuoControlReplyTriggerPortFiredEvent,NULL,0);

				free(compositionIdentifier);
				free(portIdentifier);
				break;
			}
			case VuoControlRequestInputPortValueModify:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *valueAsString = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				persistentState->nodeRegistry->setPortValue(&compositionState, portIdentifier, valueAsString);

				sendControlReply(VuoControlReplyInputPortValueModified,NULL,0);

				free(compositionIdentifier);
				free(portIdentifier);
				free(valueAsString);
				break;
			}
			case VuoControlRequestPublishedInputPortNamesRetrieve:
			{
				int count = getPublishedInputPortCount();
				char **names = getPublishedInputPortNames();

				zmq_msg_t messages[count];
				for (int i = 0; i < count; ++i)
					vuoInitMessageWithString(&messages[i], names[i]);

				sendControlReply(VuoControlReplyPublishedInputPortNamesRetrieved,messages,count);
				break;
			}
			case VuoControlRequestPublishedOutputPortNamesRetrieve:
			{
				int count = getPublishedOutputPortCount();
				char **names = getPublishedOutputPortNames();

				zmq_msg_t messages[count];
				for (int i = 0; i < count; ++i)
					vuoInitMessageWithString(&messages[i], names[i]);

				sendControlReply(VuoControlReplyPublishedOutputPortNamesRetrieved,messages,count);
				break;
			}
			case VuoControlRequestPublishedInputPortTypesRetrieve:
			{
				int count = getPublishedInputPortCount();
				char **names = getPublishedInputPortTypes();

				zmq_msg_t messages[count];
				for (int i = 0; i < count; ++i)
					vuoInitMessageWithString(&messages[i], names[i]);

				sendControlReply(VuoControlReplyPublishedInputPortTypesRetrieved,messages,count);
				break;
			}
			case VuoControlRequestPublishedOutputPortTypesRetrieve:
			{
				int count = getPublishedOutputPortCount();
				char **names = getPublishedOutputPortTypes();

				zmq_msg_t messages[count];
				for (int i = 0; i < count; ++i)
					vuoInitMessageWithString(&messages[i], names[i]);

				sendControlReply(VuoControlReplyPublishedOutputPortTypesRetrieved,messages,count);
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
					char *newDetails = mergeEnumDetails(types[i], names[i]);
					if (newDetails)
						names[i] = newDetails;
					vuoInitMessageWithString(&messages[i], names[i]);
					if (newDetails)
						free(newDetails);
				}

				sendControlReply(VuoControlReplyPublishedInputPortDetailsRetrieved,messages,count);
				break;
			}
			case VuoControlRequestPublishedOutputPortDetailsRetrieve:
			{
				int count = getPublishedOutputPortCount();
				char **names = getPublishedOutputPortDetails();

				zmq_msg_t messages[count];
				for (int i = 0; i < count; ++i)
					vuoInitMessageWithString(&messages[i], names[i]);

				sendControlReply(VuoControlReplyPublishedOutputPortDetailsRetrieved,messages,count);
				break;
			}
			case VuoControlRequestPublishedInputPortFireEvent:
			{
				int count = vuoReceiveInt(zmqControl, NULL);

				char **names = (char **)malloc(count * sizeof(char *));
				for (int i = 0; i < count; ++i)
					names[i] = vuoReceiveAndCopyString(zmqControl, NULL);

				firePublishedInputPortEvent(names, count);

				for (int i = 0; i < count; ++i)
					free(names[i]);
				free(names);

				sendControlReply(VuoControlReplyPublishedInputPortFiredEvent,NULL,0);
				break;
			}
			case VuoControlRequestPublishedInputPortValueModify:
			{
				vuoAddCompositionStateToThreadLocalStorage(&compositionState);

				while (VuoTelemetry_hasMoreToReceive(zmqControl))
				{
					char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
					char *valueAsString = vuoReceiveAndCopyString(zmqControl, NULL);

					setPublishedInputPortValue(portIdentifier, valueAsString);

					free(portIdentifier);
					free(valueAsString);
				}

				sendControlReply(VuoControlReplyPublishedInputPortValueModified,NULL,0);

				vuoRemoveCompositionStateFromThreadLocalStorage();
				break;
			}
			case VuoControlRequestPublishedInputPortValueRetrieve:
			{
				vuoAddCompositionStateToThreadLocalStorage(&compositionState);

				bool shouldUseInterprocessSerialization = vuoReceiveBool(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *valueAsString = getPublishedInputPortValue(portIdentifier, shouldUseInterprocessSerialization);
				free(portIdentifier);
				zmq_msg_t messages[1];
				vuoInitMessageWithString(&messages[0], valueAsString);
				free(valueAsString);
				sendControlReply(VuoControlReplyPublishedInputPortValueRetrieved,messages,1);

				vuoRemoveCompositionStateFromThreadLocalStorage();
				break;
			}
			case VuoControlRequestPublishedOutputPortValueRetrieve:
			{
				vuoAddCompositionStateToThreadLocalStorage(&compositionState);

				bool shouldUseInterprocessSerialization = vuoReceiveBool(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *valueAsString = getPublishedOutputPortValue(portIdentifier, shouldUseInterprocessSerialization);
				free(portIdentifier);
				zmq_msg_t messages[1];
				vuoInitMessageWithString(&messages[0], valueAsString);
				free(valueAsString);
				sendControlReply(VuoControlReplyPublishedOutputPortValueRetrieved,messages,1);

				vuoRemoveCompositionStateFromThreadLocalStorage();
				break;
			}
			case VuoControlRequestInputPortTelemetrySubscribe:
			case VuoControlRequestOutputPortTelemetrySubscribe:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				subscribeToPortDataTelemetry(compositionState.compositionIdentifier, portIdentifier);

				char *summary = persistentState->nodeRegistry->getPortSummary(&compositionState, portIdentifier);

				zmq_msg_t messages[1];
				vuoInitMessageWithString(&messages[0], summary);
				sendControlReply(control == VuoControlRequestInputPortTelemetrySubscribe ?
									 VuoControlReplyInputPortTelemetrySubscribed : VuoControlReplyOutputPortTelemetrySubscribed,
									 messages,1);

				free(compositionIdentifier);
				free(portIdentifier);
				free(summary);
				break;
			}
			case VuoControlRequestInputPortTelemetryUnsubscribe:
			case VuoControlRequestOutputPortTelemetryUnsubscribe:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);
				char *portIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				unsubscribeFromPortDataTelemetry(compositionState.compositionIdentifier, portIdentifier);

				sendControlReply(control == VuoControlRequestInputPortTelemetryUnsubscribe ?
									 VuoControlReplyInputPortTelemetryUnsubscribed : VuoControlReplyOutputPortTelemetryUnsubscribed,
									 NULL,0);

				free(compositionIdentifier);
				free(portIdentifier);
				break;
			}
			case VuoControlRequestEventTelemetrySubscribe:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				subscribeToEventTelemetry(compositionState.compositionIdentifier);

				sendControlReply(VuoControlReplyEventTelemetrySubscribed,NULL,0);

				free(compositionIdentifier);
				break;
			}
			case VuoControlRequestEventTelemetryUnsubscribe:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				unsubscribeFromEventTelemetry(compositionState.compositionIdentifier);

				sendControlReply(VuoControlReplyEventTelemetryUnsubscribed,NULL,0);

				free(compositionIdentifier);
				break;
			}
			case VuoControlRequestAllTelemetrySubscribe:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				subscribeToAllTelemetry(compositionState.compositionIdentifier);

				sendControlReply(VuoControlReplyAllTelemetrySubscribed,NULL,0);

				free(compositionIdentifier);
				break;
			}
			case VuoControlRequestAllTelemetryUnsubscribe:
			{
				char *compositionIdentifier = vuoReceiveAndCopyString(zmqControl, NULL);

				compositionState.compositionIdentifier = persistentState->nodeRegistry->defaultToTopLevelCompositionIdentifier(compositionIdentifier);
				unsubscribeFromAllTelemetry(compositionState.compositionIdentifier);

				sendControlReply(VuoControlReplyAllTelemetryUnsubscribed,NULL,0);

				free(compositionIdentifier);
				break;
			}
		}
	});

	dispatch_source_set_cancel_handler(controlTimer, ^{

		persistentState->runtimeState->breakOutOfEventLoop();
		dispatch_semaphore_signal(controlCanceled);

	});

	dispatch_resume(controlTimer);
}

/**
 * Cancels the control timer.
 */
void VuoRuntimeCommunicator::stopListeningForControl(void)
{
	if (! zmqControl)
		return;

	dispatch_source_cancel(controlTimer);
}

/**
 * Breaks out of the current `zmq_poll()` call, which is listening for the next control message.
 */
void VuoRuntimeCommunicator::interruptListeningForControl(void)
{
	if (! zmqSelfSend)
		return;

	char z = 0;
	zmq_msg_t message;
	zmq_msg_init_size(&message, sizeof z);
	memcpy(zmq_msg_data(&message), &z, sizeof z);
	if (zmq_send(zmqSelfSend, &message, 0) != 0)
		VUserLog("Couldn't break: %s (%d)", zmq_strerror(errno), errno);
	zmq_msg_close(&message);
}

/**
 * Cleans up after the canceled control timer.
 */
void VuoRuntimeCommunicator::cleanUpControl(void)
{
	if (! zmqControl)
		return;

	dispatch_semaphore_wait(controlCanceled, DISPATCH_TIME_FOREVER);
	dispatch_sync(controlQueue, ^{
					  zmq_close(zmqControl);
					  zmqControl = NULL;
				  });

	dispatch_release(controlTimer);
	controlTimer = NULL;

	if (zmqSelfSend)
	{
		zmq_close(zmqSelfSend);
		zmqSelfSend = NULL;
	}

	if (zmqSelfReceive)
	{
		zmq_close(zmqSelfReceive);
		zmqSelfReceive = NULL;
	}
}

/**
 * Starts asynchronously waiting to see if the runner process exits, indicated by the pipe connection being broken.
 * If it does, this function stops the composition.
 */
void VuoRuntimeCommunicator::startListeningForRunnerExit(void)
{
	if (runnerPipe < 0)
		return;

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_async(queue, ^{
					   char buf[1];
					   int ret;
					   int readError;
					   do {
						   ret = read(runnerPipe, &buf, 1);
						   readError = errno;
						   usleep(USEC_PER_SEC / 100);
					   } while (ret == -1 && readError == EAGAIN);

					   _hasZmqConnection = false;
					   persistentState->runtimeState->stopCompositionAsOrderedByComposition();
				   });
}

extern "C"
{
/**
 * C wrapper for VuoRuntimeCommunicator::sendNodeExecutionStarted().
 */
void vuoSendNodeExecutionStarted(VuoCompositionState *compositionState, const char *nodeIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->communicator->sendNodeExecutionStarted(compositionState->compositionIdentifier, nodeIdentifier);
}

/**
 * C wrapper for VuoRuntimeCommunicator::sendNodeExecutionFinished().
 */
void vuoSendNodeExecutionFinished(VuoCompositionState *compositionState, const char *nodeIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->communicator->sendNodeExecutionFinished(compositionState->compositionIdentifier, nodeIdentifier);
}

/**
 * C wrapper for VuoRuntimeCommunicator::sendInputPortsUpdated().
 */
void vuoSendInputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool receivedEvent, bool receivedData, const char *portDataSummary)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->communicator->sendInputPortsUpdated(compositionState->compositionIdentifier, portIdentifier, receivedEvent, receivedData, portDataSummary);
}

/**
 * C wrapper for VuoRuntimeCommunicator::sendOutputPortsUpdated().
 *
 * @version200Changed{Added `sentEvent` argument.}
 */
void vuoSendOutputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool sentEvent, bool sentData, const char *portDataSummary)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->communicator->sendOutputPortsUpdated(compositionState->compositionIdentifier, portIdentifier, sentEvent, sentData, portDataSummary);
}

/**
 * C wrapper for VuoRuntimeCommunicator::sendPublishedOutputPortsUpdated().
 */
void vuoSendPublishedOutputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool sentData, const char *portDataSummary)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->communicator->sendPublishedOutputPortsUpdated(portIdentifier, sentData, portDataSummary);
}

/**
 * C wrapper for VuoRuntimeCommunicator::sendEventFinished().
 *
 * @version200New
 */
void vuoSendEventFinished(VuoCompositionState *compositionState, unsigned long eventId)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	NodeContext *compositionContext = runtimeState->persistentState->nodeRegistry->getCompositionContext(compositionState->compositionIdentifier);
	return runtimeState->persistentState->communicator->sendEventFinished(eventId, compositionContext);
}

/**
 * C wrapper for VuoRuntimeCommunicator::sendEventDropped().
 */
void vuoSendEventDropped(VuoCompositionState *compositionState, const char *portIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->communicator->sendEventDropped(compositionState->compositionIdentifier, portIdentifier);
}

/**
 * C wrapper for VuoRuntimeCommunicator::sendError().
 */
void vuoSendError(VuoCompositionState *compositionState, const char *message)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->communicator->sendError(message);
}

/**
 * C wrapper for VuoRuntimeCommunicator::shouldSendPortDataTelemetry().
 */
bool vuoShouldSendPortDataTelemetry(VuoCompositionState *compositionState, const char *portIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->communicator->shouldSendPortDataTelemetry(compositionState->compositionIdentifier, portIdentifier);
}

/**
 * Returns the value of the input port, serialized to string.
 */
char * vuoGetInputPortString(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->nodeRegistry->getPortValue(compositionState, portIdentifier, shouldUseInterprocessSerialization);
}

/**
 * Returns the value of the output port, serialized to string.
 */
char * vuoGetOutputPortString(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->nodeRegistry->getPortValue(compositionState, portIdentifier, shouldUseInterprocessSerialization);
}

}  // extern "C"
