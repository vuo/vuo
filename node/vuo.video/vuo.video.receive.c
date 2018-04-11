/**
 * @file
 * vuo.video.receive node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoVideoFrame.h"
#include "VuoQTCapture.h"

VuoModuleMetadata({
					 "title" : "Receive Live Video",
					  "keywords" : [
						  "quicktime", "qt",
						  "firewire", "1394", "usb", "iSight", "FaceTime HD",
						  "iOS", "iPhone", "iPad", "Lightning",
						  "camera", "capture", "streaming", "record"
					  ],
					 "version" : "1.0.3",
					 "dependencies" : [
						 "VuoQTCapture"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "ShowCamera.vuo", "ShowInstantReplay.vuo" ]
					 }
				 });

struct nodeInstanceData
{
	VuoVideoInputDevice device;
	VuoQTCapture *capture;
	bool capturing;
};

static void updateDevice(struct nodeInstanceData *context, VuoVideoInputDevice newDevice, VuoOutputTrigger(receivedFrame, VuoVideoFrame))
{
	if (context->capture)
	{
		VuoRelease(context->capture);
		context->capture = NULL;
		VuoVideoInputDevice_release(context->device);
	}

	context->device = newDevice;
	VuoVideoInputDevice_retain(context->device);

	context->capture = VuoQTCapture_make(newDevice, receivedFrame);
	VuoRetain(context->capture);

	if (context->capturing)
		VuoQTCapture_startListening(context->capture);
}


struct nodeInstanceData *nodeInstanceInit
(
	VuoInputData(VuoVideoInputDevice) device
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoVideoInputDevice) device,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	VuoQTCapture_setCallback((*context)->capture, receivedFrame);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoVideoInputDevice) device,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	if (!VuoVideoInputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device, receivedFrame);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoVideoInputDevice) device,
		VuoInputEvent({"eventBlocking":"none"}) start,
		VuoInputEvent({"eventBlocking":"none"}) stop,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	if (!(*context)->capture || !VuoVideoInputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device, receivedFrame);

	if (start)
	{
		VuoQTCapture_startListening((*context)->capture);
		(*context)->capturing = true;
	}

	if (stop)
	{
		VuoQTCapture_stopListening((*context)->capture);
		(*context)->capturing = false;
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoQTCapture_setCallback((*context)->capture, NULL);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->capture)
	{
		VuoRelease((*context)->capture);
		VuoVideoInputDevice_release((*context)->device);
	}
}
