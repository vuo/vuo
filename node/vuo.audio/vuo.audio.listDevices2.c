/**
 * @file
 * vuo.audio.listDevices2 node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					 "title" : "List Audio Devices",
					 "keywords" : [ "sound", "input", "microphone", "music", "listen" ],
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoAudio"
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void *nodeInstanceInit()
{
	VuoAudio_use();
	return NULL;
}

void nodeInstanceTriggerStart
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoAudioInputDevice),
	VuoOutputTrigger(outputDevices, VuoList_VuoAudioOutputDevice)
)
{
	VuoAudio_addDevicesChangedTriggers(inputDevices, outputDevices);
}

void nodeInstanceEvent
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoAudioInputDevice),
	VuoOutputTrigger(outputDevices, VuoList_VuoAudioOutputDevice)
)
{
}

void nodeInstanceTriggerStop
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoAudioInputDevice),
	VuoOutputTrigger(outputDevices, VuoList_VuoAudioOutputDevice)
)
{
	VuoAudio_removeDevicesChangedTriggers(inputDevices, outputDevices);
}

void nodeInstanceFini
(
	VuoInstanceData(void *) nodeInstanceData
)
{
	VuoAudio_disuse();
}
