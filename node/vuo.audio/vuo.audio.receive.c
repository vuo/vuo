/**
 * @file
 * vuo.audio.receive node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"
#include "VuoAudioSamples.h"

VuoModuleMetadata({
					  "title" : "Receive Live Audio",
					  "keywords" : [ "sound", "input", "microphone", "music", "listen", "device" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "exampleCompositions" : [ "ShowLiveAudioWaveform.vuo", "VisualizeLoudness.vuo" ],
						  "isInterface" : true
					  }
				 });

VuoAudioIn nodeInstanceInit
(
		VuoInputData(VuoAudioInputDevice) device
)
{
	return VuoAudioIn_getShared(device);
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(VuoAudioIn) context,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
)
{
	VuoAudioIn_addTrigger(*context, receivedChannels);
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoAudioIn) context,
		VuoInputData(VuoAudioInputDevice) device,
		VuoInputEvent(VuoPortEventBlocking_Wall, device) deviceEvent,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples, VuoPortEventThrottling_Drop)
)
{
	if (deviceEvent)
	{
		VuoAudioIn_removeTrigger(*context, receivedChannels);
		*context = VuoAudioIn_getShared(device);
		VuoAudioIn_addTrigger(*context, receivedChannels);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(VuoAudioIn) context,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
)
{
	VuoAudioIn_removeTrigger(*context, receivedChannels);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoAudioIn) context
)
{
}
