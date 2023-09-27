/**
 * @file
 * vuo.osc.send node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOsc.h"

VuoModuleMetadata({
					  "title" : "Send OSC Messages",
					  "keywords" : [ "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					  "version" : "1.0.1",
					  "dependencies" : [
						  "VuoOsc"
					  ],
					  "node": {
						  "exampleCompositions" : [ ],
					  }
				 });


struct nodeInstanceData
{
	VuoOscOutputDevice device;
	VuoOscOut manager;
};

static void updateDevice(struct nodeInstanceData *context, VuoOscOutputDevice newDevice)
{
	VuoOscOutputDevice_release(context->device);
	context->device = newDevice;
	VuoOscOutputDevice_retain(context->device);

	VuoOscOut_disuseShared(context->manager);
	context->manager = VuoOscOut_useShared(newDevice);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoOscOutputDevice) device
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	updateDevice(context, device);

	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoOscOutputDevice) device,
		VuoInputData(VuoList_VuoOscMessage) sendMessages,
		VuoInputEvent({"eventBlocking":"none", "data":"sendMessages"}) sendMessagesEvent
)
{
	if (!VuoOscOutputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device);

	if (sendMessagesEvent)
		VuoOscOut_sendMessages((*context)->manager, sendMessages);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoOscOutputDevice_release((*context)->device);
	VuoOscOut_disuseShared((*context)->manager);
}
