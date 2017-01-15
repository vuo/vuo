/**
 * @file
 * vuo.audio.send node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					  "title" : "Send Live Audio",
					  "keywords" : [ "sound", "play", "speaker", "music", "samples", "device" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "exampleCompositions" : [ "PlayAudioFile.vuo", "PlayAudioWave.vuo", "PanAudio.vuo" ],
						  "isInterface" : true
					  }
				 });


struct nodeInstanceData
{
	VuoAudioOutputDevice device;
	VuoAudioIn audioManager;
};

static void updateDevice(struct nodeInstanceData *context, VuoAudioOutputDevice newDevice)
{
	VuoAudioOutputDevice_release(context->device);
	context->device = newDevice;
	VuoAudioOutputDevice_retain(context->device);

	VuoRelease(context->audioManager);
	context->audioManager = VuoAudioOut_getShared(newDevice);
	VuoRetain(context->audioManager);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoAudioOutputDevice) device
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	updateDevice(context, device);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	VuoAudioOut_addTrigger((*context)->audioManager, requestedChannels);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoAudioOutputDevice) device,
		VuoInputData(VuoList_VuoAudioSamples) sendChannels,
		VuoInputEvent({"eventBlocking":"none","data":"sendChannels"}) sendChannelsEvent,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	if (! VuoAudioOutputDevice_areEqual(device, (*context)->device))
	{
		VuoAudioOut_removeTrigger((*context)->audioManager, requestedChannels);
		updateDevice(*context, device);
		VuoAudioOut_addTrigger((*context)->audioManager, requestedChannels);
	}

	if (sendChannelsEvent)
		VuoAudioOut_sendChannels((*context)->audioManager, sendChannels, context);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	VuoAudioOut_removeTrigger((*context)->audioManager, requestedChannels);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoAudioOutputDevice_release((*context)->device);
	VuoRelease((*context)->audioManager);
}
