/**
 * @file
 * VuoRuntimeCommunicator interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompositionState.h"
#include "VuoTelemetry.hh"

class VuoRuntimePersistentState;
struct NodeContext;

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

	set<string> compositionsSendingAllTelemetry;  ///< Composition identifiers from which all telemetry should be sent.
	set<string> compositionsSendingEventTelemetry;  ///< Composition identifiers for which all telemetry about events (not including data) should be sent.
	map<string, set<string> > portsSendingDataTelemetry;  ///< Composition and port identifiers for which data-and-event telemetry should be sent.

	VuoRuntimePersistentState *persistentState;  ///< Reference to the parent VuoRuntimePersistentState.

	void sendControlReply(enum VuoControlReply reply, zmq_msg_t *messages, unsigned int messageCount);
	void sendTelemetry(enum VuoTelemetry type, zmq_msg_t *messages, unsigned int messageCount);

	static char * mergeEnumDetails(string type, const char *details);

	void subscribeToPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifer);
	void unsubscribeFromPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifer);
	bool isSubscribedToPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifer);

	void subscribeToEventTelemetry(const char *compositionIdentifier);
	void unsubscribeFromEventTelemetry(const char *compositionIdentifier);
	bool isSubscribedToEventTelemetry(const char *compositionIdentifier);

	void subscribeToAllTelemetry(const char *compositionIdentifier);
	void unsubscribeFromAllTelemetry(const char *compositionIdentifier);
	bool isSubscribedToAllTelemetry(const char *compositionIdentifier);

	void sendHeartbeat(bool blocking = false);

	/// @{
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
	typedef void (*firePublishedInputPortEventType)(const char *const *names, unsigned int count);
	firePublishedInputPortEventType firePublishedInputPortEvent;
	typedef void (*setPublishedInputPortValueType)(const char *portIdentifier, const char *valueAsString);
	setPublishedInputPortValueType setPublishedInputPortValue;
	typedef char * (*getPublishedInputPortValueType)(const char *portIdentifier, int shouldUseInterprocessSerialization);
	getPublishedInputPortValueType getPublishedInputPortValue;
	typedef char * (*getPublishedOutputPortValueType)(const char *portIdentifier, int shouldUseInterprocessSerialization);
	getPublishedOutputPortValueType getPublishedOutputPortValue;
	/// @}

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

	void sendNodeExecutionStarted(const char *compositionIdentifier, const char *nodeIdentifier);
	void sendNodeExecutionFinished(const char *compositionIdentifier, const char *nodeIdentifier);
	void sendInputPortsUpdated(const char *compositionIdentifier, const char *portIdentifier, bool receivedEvent, bool receivedData, const char *portDataSummary);
	void sendOutputPortsUpdated(const char *compositionIdentifier, const char *portIdentifier, bool sentEvent, bool sentData, const char *portDataSummary);
	void sendPublishedOutputPortsUpdated(const char *portIdentifier, bool sentData, const char *portDataSummary);
	void sendEventFinished(unsigned long eventId, NodeContext *compositionContext);
	void sendEventDropped(const char *compositionIdentifier, const char *portIdentifier);
	void sendError(const char *message);
	void sendStopRequested(void);
	void sendCompositionStoppingAndCloseControl(void);

	bool shouldSendPortDataTelemetry(const char *compositionIdentifier, const char *portIdentifier);
};

extern "C"
{
void vuoSendNodeExecutionStarted(VuoCompositionState *compositionState, const char *nodeIdentifier);
void vuoSendNodeExecutionFinished(VuoCompositionState *compositionState, const char *nodeIdentifier);
void vuoSendInputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool receivedEvent, bool receivedData, const char *portDataSummary);
void vuoSendOutputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool sentEvent, bool sentData, const char *portDataSummary);
void vuoSendPublishedOutputPortsUpdated(VuoCompositionState *compositionState, const char *portIdentifier, bool sentData, const char *portDataSummary);
void vuoSendEventFinished(VuoCompositionState *compositionState, unsigned long eventId);
void vuoSendEventDropped(VuoCompositionState *compositionState, const char *portIdentifier);
bool vuoShouldSendPortDataTelemetry(VuoCompositionState *compositionState, const char *portIdentifier);
char * vuoGetInputPortString(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization);
char * vuoGetOutputPortString(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization);
}
