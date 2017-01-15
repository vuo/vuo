/**
 * @file
 * vuo.serial.receive node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSerial.h"

VuoModuleMetadata({
					 "title" : "Receive Serial Data",
					 "keywords" : [
						 "read", "open", "listen",
						 "arduino", "rs232", "rs-232", "usb", "modem"
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoSerialIO"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "LogTextFromSerialDevice.vuo" ],
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

	if (VuoSerialDevice_realize(newDevice, &context->device))
	{
		VuoSerialDevice_retain(context->device);
		context->s = VuoSerial_getShared(context->device.path);
		VuoRetain(context->s);
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

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedData, VuoData)
)
{
	VuoSerial_addReceiveTrigger((*context)->s, receivedData);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoSerialDevice) device,
		VuoOutputTrigger(receivedData, VuoData)
)
{
	if (!(*context)->s || !VuoSerialDevice_areEqual(device, (*context)->device))
	{
		VuoSerial_removeReceiveTrigger((*context)->s, receivedData);
		updateDevice(*context, device);
		VuoSerial_addReceiveTrigger((*context)->s, receivedData);
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoSerialDevice) device,
		VuoOutputTrigger(receivedData, VuoData)
)
{
	if (!(*context)->s || !VuoSerialDevice_areEqual(device, (*context)->device))
	{
		VuoSerial_removeReceiveTrigger((*context)->s, receivedData);
		updateDevice(*context, device);
		VuoSerial_addReceiveTrigger((*context)->s, receivedData);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedData, VuoData)
)
{
	VuoSerial_removeReceiveTrigger((*context)->s, receivedData);
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
