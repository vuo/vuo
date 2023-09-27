/**
 * @file
 * vuo.video.receive2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoVideoFrame.h"
#include "VuoVideoCapture.h"

VuoModuleMetadata({
					 "title" : "Receive Live Video",
					  "keywords" : [
						  "quicktime", "qt",
						  "firewire", "1394", "usb", "iSight", "FaceTime HD",
						  "iOS", "iPhone", "iPad", "Lightning", "tethered",
						  "camera", "capture", "streaming", "record"
					  ],
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoVideoCapture"
					 ],
					 "node": {
						 "exampleCompositions" : [ "ShowLiveVideo.vuo", "ShowInstantReplay.vuo" ]
					 }
				 });

struct nodeInstanceData
{
	VuoVideoInputDevice device;
	VuoVideoCapture *capture;
	VuoInteger currentWidth, currentHeight;
	bool triggersEnabled;
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

	context->capture = VuoVideoCapture_make(newDevice, receivedFrame);
	VuoRetain(context->capture);

	context->currentWidth = 0;
	context->currentHeight = 0;

	VuoVideoCapture_startListening(context->capture);
}

static void updateSizeIfNeeded(struct nodeInstanceData *context, VuoInteger width, VuoInteger height)
{
	if (width  != context->currentWidth
	 || height != context->currentHeight)
	{
		VuoVideoCapture_setSize(context->capture, width, height);

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
		VuoInputData(VuoInteger) width,
		VuoInputData(VuoInteger) height,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	(*context)->triggersEnabled = true;

	if (!VuoVideoInputDevice_areEqual(device, (*context)->device))
	{
		updateDevice(*context, device, receivedFrame);
		updateSizeIfNeeded(*context, width, height);
	}

	VuoVideoCapture_setCallback((*context)->capture, receivedFrame);
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
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	if (!(*context)->triggersEnabled)
		return;

	if (!(*context)->capture || !VuoVideoInputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device, receivedFrame);

	updateSizeIfNeeded(*context, width, height);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoVideoCapture_setCallback((*context)->capture, NULL);
	(*context)->triggersEnabled = false;
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
