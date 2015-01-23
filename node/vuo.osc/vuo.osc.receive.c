/**
 * @file
 * vuo.osc.receive node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOsc.h"

VuoModuleMetadata({
					 "title" : "Receive OSC Messages",
					 "keywords" : [ "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoOsc"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions": [ "ReceiveOsc.vuo" ]
					 }
				 });


VuoOscIn nodeInstanceInit
(
		VuoInputData(VuoInteger) udpPort
)
{
	VuoOscIn oi = VuoOscIn_make(udpPort);
	VuoRetain(oi);
	return oi;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(VuoOscIn) context,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	VuoOscIn_enableTriggers(*context, receivedMessage);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(VuoOscIn) context,
		VuoInputData(VuoInteger) udpPort,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	VuoOscIn_disableTriggers(*context);
	VuoRelease(*context);
	*context = VuoOscIn_make(udpPort);
	VuoRetain(*context);
	VuoOscIn_enableTriggers(*context, receivedMessage);
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoOscIn) context,
		VuoInputData(VuoInteger, {"default":0}) udpPort,
		VuoInputEvent(VuoPortEventBlocking_Wall, udpPort) udpPortEvent,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	if (udpPortEvent)
	{
		VuoOscIn_disableTriggers(*context);
		VuoRelease(*context);
		*context = VuoOscIn_make(udpPort);
		VuoRetain(*context);
		VuoOscIn_enableTriggers(*context, receivedMessage);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(VuoOscIn) context
)
{
	VuoOscIn_disableTriggers(*context);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoOscIn) context
)
{
	VuoRelease(*context);
}
