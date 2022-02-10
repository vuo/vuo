/**
 * @file
 * vuo.serial.send node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoSerial.h"

VuoModuleMetadata({
					  "title" : "Send Serial Data",
					  "keywords" : [
						  "write", "open",
						  "arduino", "rs232", "rs-232", "usb", "modem"
					  ],
					  "version" : "1.0.1",
					  "dependencies" : [
						  "VuoSerialIO"
					  ],
					  "node": {
						  "exampleCompositions" : [ "SendTextToSerialDevice.vuo" ],
					  }
				 });

struct nodeInstanceData
{
	VuoSerialDevice device;
	VuoSerial s;
};

static void updateDevice(struct nodeInstanceData *context, VuoSerialDevice newDevice)
{
	if (context->s)
	{
		VuoRelease(context->s);
		context->s = NULL;
		VuoSerialDevice_release(context->device);
	}

	VuoSerialDevice realizedDevice;
	if (VuoSerialDevice_realize(newDevice, &realizedDevice))
	{
		context->device = newDevice;
		VuoSerialDevice_retain(context->device);
		context->s = VuoSerial_getShared(realizedDevice.path);
		VuoRetain(context->s);

		VuoSerialDevice_retain(realizedDevice);
		VuoSerialDevice_release(realizedDevice);
	}
}

struct nodeInstanceData *nodeInstanceInit
(
		VuoInputData(VuoSerialDevice) device
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	updateDevice(context, device);

	return context;
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoSerialDevice) device
)
{
	if (!VuoSerialDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device);
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoData) sendData,
	VuoInputEvent({"eventBlocking":"none", "data":"sendData"}) sendDataEvent,
	VuoInputData(VuoSerialDevice) device)
{
	if (!(*context)->s || !VuoSerialDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device);

	if (sendDataEvent)
		VuoSerial_sendData((*context)->s, sendData);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->s)
	{
		VuoRelease((*context)->s);
		VuoSerialDevice_release((*context)->device);
	}
}
