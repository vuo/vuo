/**
 * @file
 * vuo.video.receive node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
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
						  "iOS", "iPhone", "iPad", "Lightning", "tethered",
						  "camera", "capture", "streaming", "record"
					  ],
					 "version" : "1.1.0",
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
	VuoInteger currentWidth, currentHeight;
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

static void updateSizeIfNeeded(struct nodeInstanceData *context, VuoInteger width, VuoInteger height)
{
	if (width  != context->currentWidth
	 || height != context->currentHeight)
	{
		VuoQTCapture_setSize(context->capture, width, height);

		context->currentWidth = width;
		context->currentHeight = height;
	}
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
		VuoInputData(VuoInteger) width,
		VuoInputData(VuoInteger) height,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	if (!VuoVideoInputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device, receivedFrame);

	updateSizeIfNeeded(*context, width, height);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoVideoInputDevice) device,
		VuoInputData(VuoInteger, {"default":1920, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256, "auto":0, "autoSupersedesDefault":true}) width,
		VuoInputData(VuoInteger, {"default":1080, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256, "auto":0, "autoSupersedesDefault":true}) height,
		VuoInputEvent({"eventBlocking":"none"}) start,
		VuoInputEvent({"eventBlocking":"none"}) stop,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	if (!(*context)->capture || !VuoVideoInputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device, receivedFrame);

	updateSizeIfNeeded(*context, width, height);

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
