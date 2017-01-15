/**
 * @file
 * vuo.video.listDevices node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoQTCapture.h"
#include "VuoList_VuoVideoInputDevice.h"

VuoModuleMetadata({
					 "title" : "List Video Devices",
					  "keywords" : [
						  "quicktime", "qt",
						  "video",
					  ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoQTCapture"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ ]
					 }
				 });

void* nodeInstanceInit()
{
	return NULL;
}

void nodeInstanceTriggerStart
(
	VuoInstanceData(void*) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoVideoInputDevice)
)
{
	VuoQTCapture_addOnDevicesChangedCallback(inputDevices);
}

void nodeInstanceEvent(
	VuoInstanceData(void*) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoVideoInputDevice)
	)
{

}

void nodeInstanceTriggerStop(
	VuoInstanceData(void*) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoVideoInputDevice)
	)
{
	VuoQTCapture_removeOnDevicesChangedCallback(inputDevices);
}

void nodeInstanceFini(
	VuoInstanceData(void*) nodeInstanceData
	)
{
}
