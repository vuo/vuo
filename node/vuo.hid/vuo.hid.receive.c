/**
 * @file
 * vuo.hid.receive node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoHid.h"

VuoModuleMetadata({
    "title": "Receive HID Controls",
    "keywords": [
        "read", "open", "listen",
        "USB", "Bluetooth",
    ],
    "version": "1.0.0",
    "dependencies": [
        "VuoHidIo",
    ],
    "node": {
        "exampleCompositions": [ "MoveDotsWithTwoMice.vuo", "MoveIcosahedronWithSpacenavigator.vuo", "ListKeypresses.vuo" ],
    },
});

struct nodeInstanceData
{
	VuoHidDevice device;
	bool exclusive;
	VuoHid s;
	bool triggersEnabled;
};

static void updateDevice(struct nodeInstanceData *context, VuoHidDevice newDevice, VuoBoolean exclusive)
{
	VuoHidDevice_release(context->device);
	context->device = newDevice;
	VuoHidDevice_retain(context->device);

	context->exclusive = exclusive;

	VuoRelease(context->s);
	context->s = VuoHid_make(context->device, exclusive);
	VuoRetain(context->s);
}

struct nodeInstanceData *nodeInstanceInit
(
		VuoInputData(VuoHidDevice) device,
		VuoInputData(VuoBoolean) exclusive
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	updateDevice(context, device, exclusive);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedControl, VuoHidControl)
)
{
	(*context)->triggersEnabled = true;
	VuoHid_addReceiveTrigger((*context)->s, receivedControl);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoHidDevice) device,
		VuoInputData(VuoBoolean) exclusive,
		VuoOutputTrigger(receivedControl, VuoHidControl)
)
{
	if (!(*context)->s || !VuoHidDevice_areEqual(device, (*context)->device) || (*context)->exclusive != exclusive)
	{
		VuoHid_removeReceiveTrigger((*context)->s, receivedControl);
		updateDevice(*context, device, exclusive);
		VuoHid_addReceiveTrigger((*context)->s, receivedControl);
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoHidDevice) device,
		VuoInputData(VuoBoolean, {"default":false}) exclusive,
		VuoOutputTrigger(receivedControl, VuoHidControl)
)
{
	if (!(*context)->triggersEnabled)
		return;

	if (!(*context)->s || !VuoHidDevice_areEqual(device, (*context)->device) || (*context)->exclusive != exclusive)
	{
		VuoHid_removeReceiveTrigger((*context)->s, receivedControl);
		updateDevice(*context, device, exclusive);
		VuoHid_addReceiveTrigger((*context)->s, receivedControl);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedControl, VuoHidControl)
)
{
	VuoHid_removeReceiveTrigger((*context)->s, receivedControl);
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->s)
	{
		VuoRelease((*context)->s);
		VuoHidDevice_release((*context)->device);
	}
}
