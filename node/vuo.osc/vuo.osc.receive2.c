/**
 * @file
 * vuo.osc.receive2 node implementation.
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
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoOsc"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions": [ "ReceiveOsc.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoOscInputDevice device;
	VuoOscIn manager;
};

static void updateDevice(struct nodeInstanceData *context, VuoOscInputDevice device)
{
	VuoOscInputDevice_release(context->device);
	context->device = device;
	VuoOscInputDevice_retain(context->device);

	VuoRelease(context->manager);
	context->manager = VuoOscIn_make(context->device);
	VuoRetain(context->manager);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoOscInputDevice) device
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	updateDevice(context, device);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	VuoOscIn_enableTriggers((*context)->manager, receivedMessage);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoOscInputDevice) device,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	if (!(*context)->manager || !VuoOscInputDevice_areEqual(device, (*context)->device))
	{
		VuoOscIn_disableTriggers((*context)->manager);
		updateDevice(*context, device);
		VuoOscIn_enableTriggers((*context)->manager, receivedMessage);
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoOscInputDevice) device,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	if (!(*context)->manager || !VuoOscInputDevice_areEqual(device, (*context)->device))
	{
		VuoOscIn_disableTriggers((*context)->manager);
		updateDevice(*context, device);
		VuoOscIn_enableTriggers((*context)->manager, receivedMessage);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoOscIn_disableTriggers((*context)->manager);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->manager)
	{
		VuoRelease((*context)->manager);
		VuoOscInputDevice_release((*context)->device);
	}
}
