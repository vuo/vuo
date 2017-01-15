/**
 * @file
 * vuo.video.receive node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
						  "firewire", "1394", "usb", "iSight",
						  "iOS", "iPhone", "iPad", "Lightning",
						  "camera", "capture", "streaming", "record"
					  ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoQTCapture"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "ShowCamera.vuo", "ShowInstantReplay.vuo" ]
					 }
				 });

VuoQTCapture * nodeInstanceInit
(
	VuoInputData(VuoVideoInputDevice) device
)
{
	return NULL;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(VuoQTCapture *) movie,
		VuoInputData(VuoVideoInputDevice) device,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	if( VuoQTCapture_isInitialized(*movie) )
	{
		VuoQTCapture_setCallback(*movie, receivedFrame);
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoQTCapture *) movie,
		VuoInputData(VuoVideoInputDevice) device,
		VuoInputEvent({"eventBlocking":"none","data":"device"}) deviceEvent,
		VuoInputEvent({"eventBlocking":"none"}) start,
		VuoInputEvent({"eventBlocking":"none"}) stop,
		VuoOutputTrigger(receivedFrame, VuoVideoFrame, {"eventThrottling":"drop"})
)
{
	if(deviceEvent)
	{
		VuoQtCapture_setInputDevice(*movie, device);
	}

	if(start)
	{
		if( !VuoQTCapture_isInitialized(*movie) )
			*movie = VuoQTCapture_make(device, receivedFrame);

		VuoQTCapture_startListening(*movie);
	}

	if(stop && *movie != NULL)
		VuoQTCapture_stopListening(*movie);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(VuoQTCapture *) movie
)
{
	// set callback to null!
	VuoQTCapture_setCallback(*movie, NULL);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoQTCapture *) movie
)
{
}
