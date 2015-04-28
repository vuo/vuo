/**
 * @file
 * vuo.audio.send node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					  "title" : "Send Live Audio",
					  "keywords" : [ "sound", "output", "play", "speaker", "music", "samples", "device" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "exampleCompositions" : [ "PlayAudioFile.vuo", "PlayAudioWave.vuo" ],
						  "isInterface" : true
					  }
				 });


VuoAudioOut nodeInstanceInit
(
		VuoInputData(VuoAudioOutputDevice) device
)
{
	return VuoAudioOut_getShared(device);
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(VuoAudioOut) context,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	VuoAudioOut_addTrigger(*context, requestedChannels);
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoAudioOut) context,
		VuoInputData(VuoAudioOutputDevice) device,
		VuoInputEvent(VuoPortEventBlocking_Wall, device) deviceEvent,
		VuoInputData(VuoList_VuoAudioSamples) sendChannels,
		VuoInputEvent(VuoPortEventBlocking_None, sendChannels) sendChannelsEvent,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	if (deviceEvent)
	{
		VuoAudioOut_removeTrigger(*context, requestedChannels);
		*context = VuoAudioOut_getShared(device);
		VuoAudioOut_addTrigger(*context, requestedChannels);
	}

	if (sendChannelsEvent)
		VuoAudioOut_sendChannels(*context, sendChannels, context);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(VuoAudioOut) context,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	VuoAudioOut_removeTrigger(*context, requestedChannels);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoAudioOut) context
)
{
}
