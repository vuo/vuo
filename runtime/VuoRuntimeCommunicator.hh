/**
 * @file
 * VuoRuntimeCommunicator interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoCompositionState.h"
#include "VuoTelemetry.h"

class VuoRuntimePersistentState;

/**
 * Manages communication between the runtime and the runner.
 */
class VuoRuntimeCommunicator
{
private:
	bool _hasZmqConnection;   ///< True if the @ref ZMQControl and @ref ZMQTelemetry sockets are connected to something.

	void *zmqContext;	///< The context used to initialize sockets.
	void *zmqControl;	///< The control socket. Use only on VuoControlQueue.
	void *zmqSelfReceive;	///< Used to break out of a ZMQControl poll.
	void *zmqSelfSend;		///< Used to break out of a ZMQControl poll.
	void *zmqTelemetry;		///< The telemetry socket. Use only on VuoTelemetryQueue.

	dispatch_queue_t controlQueue;  ///< Dispatch queue for protecting access to the @c ZMQControl socket.
	dispatch_queue_t telemetryQueue;  ///< Dispatch queue for protecting access to the @c ZMQTelemetry socket.
	dispatch_source_t controlTimer;  ///< Timer for receiving control messages.
	dispatch_source_t telemetryTimer;  ///< Timer for sending telemetry messages.
	dispatch_semaphore_t controlCanceled;  ///< Signaled when control events are no longer being processed.
	dispatch_semaphore_t telemetryCanceled;  ///< Signaled when telemetry events are no longer being processed.

	int runnerPipe;  ///< Pipe between the runtime (read) and the runner (write), used to detect if the runner process ends.

	bool isSendingAllTelemetry;  ///< True if all telemetry should be sent.
	bool isSendingEventTelemetry;  ///< True if all telemetry about events (not including data) should be sent.
	set<string> portsSendingDataTelemetry;  ///< Port identifiers for which data-and-event telemetry should be sent.

	VuoRuntimePersistentState *persistentState;  ///< Reference to the parent VuoRuntimePersistentState.

	void sendControlReply(enum VuoControlReply reply, zmq_msg_t *messages, unsigned int messageCount);
	void sendTelemetry(enum VuoTelemetry type, zmq_msg_t *messages, unsigned int messageCount);

	char * getInputPortSummary(const char *portIdentifier);
	char * getOutputPortSummary(const char *portIdentifier);
	static char * mergeEnumDetails(string type, const char *details);

	//@{
	/**
	 * Defined in the composition's generated code.
	 */
	typedef void (*vuoInstanceInitType)(void);
	vuoInstanceInitType vuoInstanceInit;
	typedef void (*vuoInstanceTriggerStartType)(void);
	vuoInstanceTriggerStartType vuoInstanceTriggerStart;
	typedef void (*vuoInstanceTriggerStopType)(void);
	vuoInstanceTriggerStopType vuoInstanceTriggerStop;
	typedef void (*vuoSetInputPortValueType)(const char *portIdentifier, char *valueAsString);
	vuoSetInputPortValueType vuoSetInputPortValue;
	typedef void (*fireTriggerPortEventType)(const char *portIdentifier);
	fireTriggerPortEventType fireTriggerPortEvent;
	typedef char * (*vuoGetPortValueType)(const char *portIdentifier, int serializationType);
	vuoGetPortValueType vuoGetPortValue;
	typedef unsigned int (*getPublishedInputPortCountType)(void);
	getPublishedInputPortCountType getPublishedInputPortCount;
	typedef unsigned int (*getPublishedOutputPortCountType)(void);
	getPublishedOutputPortCountType getPublishedOutputPortCount;
	typedef char ** (*getPublishedInputPortNamesType)(void);
	getPublishedInputPortNamesType getPublishedInputPortNames;
	typedef char ** (*getPublishedOutputPortNamesType)(void);
	getPublishedOutputPortNamesType getPublishedOutputPortNames;
	typedef char ** (*getPublishedInputPortTypesType)(void);
	getPublishedInputPortTypesType getPublishedInputPortTypes;
	typedef char ** (*getPublishedOutputPortTypesType)(void);
	getPublishedOutputPortTypesType getPublishedOutputPortTypes;
	typedef char ** (*getPublishedInputPortDetailsType)(void);
	getPublishedInputPortDetailsType getPublishedInputPortDetails;
	typedef char ** (*getPublishedOutputPortDetailsType)(void);
	getPublishedOutputPortDetailsType getPublishedOutputPortDetails;
	typedef void (*firePublishedInputPortEventType)(const char *name);
	firePublishedInputPortEventType firePublishedInputPortEvent;
	typedef void (*setPublishedInputPortValueType)(const char *portIdentifier, const char *valueAsString);
	setPublishedInputPortValueType setPublishedInputPortValue;
	typedef char * (*getPublishedInputPortValueType)(const char *portIdentifier, int shouldUseInterprocessSerialization);
	getPublishedInputPortValueType getPublishedInputPortValue;
	typedef char * (*getPublishedOutputPortValueType)(const char *portIdentifier, int shouldUseInterprocessSerialization);
	getPublishedOutputPortValueType getPublishedOutputPortValue;
	//@}

public:
	VuoRuntimeCommunicator(VuoRuntimePersistentState *persistentState);
	~VuoRuntimeCommunicator(void);
	void updateCompositionSymbols(void *compositionBinaryHandle);
	void openConnection(void *_zmqContext, const char *controlURL, const char *telemetryURL, int runnerPipe);
	void closeConnection(void);

	bool hasZmqConnection(void);
	dispatch_queue_t getControlQueue(void);

	void startSendingHeartbeat(void);
	void stopSendingAndCleanUpHeartbeat(void);
	void startListeningForControl(void);
	void stopListeningForControl(void);
	void interruptListeningForControl(void);
	void cleanUpControl(void);
	void startListeningForRunnerExit(void);

	void sendNodeExecutionStarted(const char *nodeIdentifier);
	void sendNodeExecutionFinished(const char *nodeIdentifier);
	void sendInputPortsUpdated(const char *portIdentifier, bool receivedEvent, bool receivedData, const char *portDataSummary);
	void sendOutputPortsUpdated(const char *portIdentifier, bool sentData, const char *portDataSummary);
	void sendPublishedOutputPortsUpdated(const char *portIdentifier, bool sentData, const char *portDataSummary);
	void sendEventDropped(const char *portIdentifier);
	void sendError(const char *message);
	void sendStopRequested(void);
	void sendCompositionStoppingAndCloseControl(void);

	bool shouldSendPortDataTelemetry(const char *portIdentifier);

	char * getInputPortString(const char *portIdentifier, bool shouldUseInterprocessSerialization);
	char * getOutputPortString(const char *portIdentifier, bool shouldUseInterprocessSerialization);
};

extern "C"
{
void vuoSendNodeExecutionStarted(VuoCompositionState *compositionState, const char *nodeIdentifier);
void vuoSendNodeExecutionFinished(VuoCompositionState *compositionState, const char *nodeIdentifier);
void vuoSendInputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool receivedEvent, bool receivedData, const char *portDataSummary);
void vuoSendOutputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool sentData, const char *portDataSummary);
void vuoSendPublishedOutputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool sentData, const char *portDataSummary);
void vuoSendEventDropped(VuoCompositionState *compositionState, const char *portIdentifier);
bool vuoShouldSendPortDataTelemetry(VuoCompositionState *compositionState, const char *portIdentifier);
char * vuoGetInputPortString(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization);
char * vuoGetOutputPortString(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization);
}
