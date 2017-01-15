/**
 * @file
 * vuo.osc.receive node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
						 "isDeprecated": true,
						 "isInterface" : true,
						 "exampleCompositions": [ "ReceiveOsc.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoInteger udpPort;
	VuoOscIn oscManager;
};

static void updatePort(struct nodeInstanceData *context, VuoInteger newUdpPort)
{
	context->udpPort = newUdpPort;

	VuoRelease(context->oscManager);
	VuoOscInputDevice device = {NULL, NULL, newUdpPort};
	context->oscManager = VuoOscIn_make(device);
	VuoRetain(context->oscManager);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoInteger) udpPort
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	updatePort(context, udpPort);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	VuoOscIn_enableTriggers((*context)->oscManager, receivedMessage);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoInteger) udpPort,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	VuoOscIn_disableTriggers((*context)->oscManager);
	updatePort(*context, udpPort);
	VuoOscIn_enableTriggers((*context)->oscManager, receivedMessage);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoInteger, {"name":"UDP Port", "default":0, "suggestedMin":0, "suggestedMax":65535}) udpPort,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	if (udpPort != (*context)->udpPort)
	{
		VuoOscIn_disableTriggers((*context)->oscManager);
		updatePort(*context, udpPort);
		VuoOscIn_enableTriggers((*context)->oscManager, receivedMessage);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoOscIn_disableTriggers((*context)->oscManager);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->oscManager);
}
