/**
 * @file
 * VuoTelemetry interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOTELEMETRY_H
#define VUOTELEMETRY_H

#include <stdbool.h>

/**
 * Control Requests, sent to the composition.
 */
enum VuoControlRequest
{
	/**
	 * Request that the composition stop executing (without serialization).
	 *
	 * Includes data message-parts:
	 *		@arg @c int timeoutInSeconds;
	 */
	VuoControlRequestCompositionStop,

	/**
	 * Request that the composition pause execution (without serialization).
	 */
	VuoControlRequestCompositionPause,

	/**
	 * Request that the composition unpause execution.
	 */
	VuoControlRequestCompositionUnpause,

	/**
	 * Request that the input port be set to the given value (converted to the port's type).
	 *
	 * Includes data message-parts:
	 *		@arg @c char *portIdentifier;
	 *		@arg @c char *valueAsString;
	 */
	VuoControlRequestInputPortValueModify,

	/**
	 * Request that the input port's value be looked up and returned.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *portIdentifier;
	 */
	VuoControlRequestInputPortValueRetrieve,

	/**
	 * Request that the output port's value be looked up and returned.
	 *
	 * Includes data message-parts:
	 *		@arg @c bool shouldUseInterprocessSerialization;
	 *		@arg @c char *portIdentifier;
	 */
	VuoControlRequestOutputPortValueRetrieve,

	/**
	 * Request that the input port's summary be looked up and returned.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *portIdentifier;
	 */
	VuoControlRequestInputPortSummaryRetrieve,

	/**
	 * Request that the output port's summary be looked up and returned.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *portIdentifier;
	 */
	VuoControlRequestOutputPortSummaryRetrieve,

	/**
	 * Request that an event be fired from the trigger port.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *portIdentifier;
	 */
	VuoControlRequestTriggerPortFireEvent,

	/**
	 * Request that the published input ports' names be looked up and returned.
	 */
	VuoControlRequestPublishedInputPortNamesRetrieve,

	/**
	 * Request that the published output ports' names be looked up and returned.
	 */
	VuoControlRequestPublishedOutputPortNamesRetrieve,

	/**
	 * Request that the published input ports' types be looked up and returned.
	 */
	VuoControlRequestPublishedInputPortTypesRetrieve,

	/**
	 * Request that the published output ports' types be looked up and returned.
	 */
	VuoControlRequestPublishedOutputPortTypesRetrieve,

	/**
	 * Request that the published input port's connected unpublished ports be looked up and returned.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *name
	 */
	VuoControlRequestPublishedInputPortConnectedIdentifiersRetrieve,

	/**
	 * Request that the published output port's connected unpublished ports be looked up and returned.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *name
	 */
	VuoControlRequestPublishedOutputPortConnectedIdentifiersRetrieve,

	/**
	 * Request that an event be fired through the published input port.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *name
	 */
	VuoControlRequestPublishedInputPortFireEvent,

	/**
	 * Request that the published input port be set to the given value (converted to the port's type).
	 *
	 * Includes data message-parts:
	 *		@arg @c char *name;
	 *		@arg @c char *valueAsString;
	 */
	VuoControlRequestPublishedInputPortValueModify,

	/**
	 * Request that the published output port's value be looked up and returned.
	 *
	 * Includes data message-parts:
	 *		@arg @c bool shouldUseInterprocessSerialization;
	 *		@arg @c char *name;
	 */
	VuoControlRequestPublishedOutputPortValueRetrieve
};

/**
 * Control Replies, sent from the composition.
 */
enum VuoControlReply
{
	/**
	 * Composition shutdown has been initiated.
	 */
	VuoControlReplyCompositionStopping,

	/**
	 * Composition has been paused.
	 */
	VuoControlReplyCompositionPaused,

	/**
	 * Composition has been unpaused.
	 */
	VuoControlReplyCompositionUnpaused,

	/**
	 * The input port's value has been set.
	 */
	VuoControlReplyInputPortValueModified,

	/**
	 * A string representation of the input port's value has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *valueAsString;
	 */
	VuoControlReplyInputPortValueRetrieved,

	/**
	 * A string representation of the output port's value has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *valueAsString;
	 */
	VuoControlReplyOutputPortValueRetrieved,

	/**
	 * A brief description of the input port's value has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *summary;
	 */
	VuoControlReplyInputPortSummaryRetrieved,

	/**
	 * A brief description of the output port's value has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *summary;
	 */
	VuoControlReplyOutputPortSummaryRetrieved,

	/**
	 * An event has been fired from the trigger port.
	 */
	VuoControlReplyTriggerPortFiredEvent,

	/**
	 * The list of published input ports' names has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg char *name0
	 *		@arg char *name1
	 *		@arg ...
	 */
	VuoControlReplyPublishedInputPortNamesRetrieved,

	/**
	 * The list of published output ports' names has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg char *name0
	 *		@arg char *name1
	 *		@arg ...
	 */
	VuoControlReplyPublishedOutputPortNamesRetrieved,

	/**
	 * The list of published input ports' types, as string representations, has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg char *type0
	 *		@arg char *type1
	 *		@arg ...
	 */
	VuoControlReplyPublishedInputPortTypesRetrieved,

	/**
	 * The list of published output ports' types, as string representations, has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg char *type0
	 *		@arg char *type1
	 *		@arg ...
	 */
	VuoControlReplyPublishedOutputPortTypesRetrieved,

	/**
	 * The list of published input ports' connected unpublished ports has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg char *identifier0
	 *		@arg char *identifier1
	 *		@arg ...
	 */
	VuoControlReplyPublishedInputPortConnectedIdentifiersRetrieved,

	/**
	 * The list of published output ports' connected unpublished ports has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg char *identifier0
	 *		@arg char *identifier1
	 *		@arg ...
	 */
	VuoControlReplyPublishedOutputPortConnectedIdentifiersRetrieved,

	/**
	 * An event has been fired through the published input port.
	 */
	VuoControlReplyPublishedInputPortFiredEvent,

	/**
	 * The published input port's value has been set.
	 */
	VuoControlReplyPublishedInputPortValueModified,

	/**
	 * A string representation of the published output port's value has been retrieved.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *valueAsString;
	 */
	VuoControlReplyPublishedOutputPortValueRetrieved
};

/**
 * Control Requests, sent to the composition loader.
 */
enum VuoLoaderControlRequest
{
	/**
	 * Request that the composition be replaced with an updated version.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *dylibPath;
	 *		@arg @c char *compositionDiff;
	 */
	VuoLoaderControlRequestCompositionReplace
};

/**
 * Control Replies, sent from the composition loader.
 */
enum VuoLoaderControlReply
{
	/**
	 * The composition has been replaced with an updated version.
	 */
	VuoLoaderControlReplyCompositionReplaced
};

/**
 * Types of published telemetry data.
 */
enum VuoTelemetry
{
	/**
	 * General information about the composition process, published every half-second.
	 *
	 * Includes data message-parts:
	 *		@arg @c unsigned long utime;
	 *		@arg @c unsigned long stime;
	 */
	VuoTelemetryStats,

	/**
	 * Published just prior to calling each node's nodeEvent/nodeInstanceEvent function.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *nodeIdentifier;
	 */
	VuoTelemetryNodeExecutionStarted,

	/**
	 * Published just after each node's nodeEvent/nodeInstanceEvent function returns.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *nodeIdentifier;
	 */
	VuoTelemetryNodeExecutionFinished,

	/**
	 * Published just after each node's nodeEvent/nodeInstanceEvent function returns
	 * (for each input port that receives an event from the node)
	 * and just after an input port's value is set in response to a
	 * VuoControlRequestInputPortValueModify message.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *portIdentifier0;
	 *		@arg @c bool receivedEvent0;
	 *		@arg @c bool receivedData0;
	 *		@arg @c char *portDataSummary0;
	 *		@arg @c char *portIdentifier1;
	 *		@arg @c bool receivedEvent1;
	 *		@arg @c bool receivedData1;
	 *		@arg @c char *portData1;
	 *		@arg @c char *portDataSummary1;
	 *		@arg @c ...
	 */
	VuoTelemetryInputPortsUpdated,

	/**
	 * Published just after each node's nodeEvent/nodeInstanceEvent function returns,
	 * (for each output port that transmits an event)
	 * and just after a trigger port fires an event.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *portIdentifier0;
	 *		@arg @c bool sentData0;
	 *		@arg @c char *portDataSummary0;
	 *		@arg @c char *portIdentifier1;
	 *		@arg @c bool sentData1;
	 *		@arg @c char *portData1;
	 *		@arg @c char *portDataSummary1;
	 *		@arg @c ...
	 */
	VuoTelemetryOutputPortsUpdated,

	/**
	 * Published when an uncaught error occurs in the composition.
	 *
	 * Includes data message-parts:
	 *		@arg @c char *message;
	 */
	VuoTelemetryError,

	/**
	 * Published when a node in the composition requests that the composition stop.
	 */
	VuoTelemetryStopRequested
};


#ifdef __cplusplus
extern "C" {
#endif

char * vuoCopyStringFromMessage(zmq_msg_t *message);
void vuoInitMessageWithString(zmq_msg_t *message, const char *string);
void vuoInitMessageWithInt(zmq_msg_t *message, int value);
void vuoInitMessageWithBool(zmq_msg_t *message, bool value);
char * vuoReceiveAndCopyString(void *socket);
unsigned long vuoReceiveUnsignedInt64(void *socket);
int vuoReceiveInt(void *socket);
bool vuoReceiveBool(void *socket);
void vuoSend(const char *name, void *socket, int type, zmq_msg_t *messages, unsigned int messageCount, bool isNonBlocking);
void vuoMemoryBarrier(void);

#ifdef __cplusplus
}
#endif

#endif
