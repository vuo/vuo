/**
 * @file
 * vuo.audio.send node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					  "title" : "Send Live Audio",
					  "keywords" : [ "sound", "play", "speaker", "music", "samples", "device" ],
					  "version" : "1.0.1",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "PlayAudioFile.vuo", "PlayAudioWave.vuo", "PanAudio.vuo", "GenerateParametricAudio.vuo" ],
					  }
				 });


struct nodeInstanceData
{
	VuoAudioOutputDevice device;
	VuoAudioIn audioManager;
	bool triggersEnabled;
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
	(*context)->triggersEnabled = true;
	VuoAudioOut_addTrigger((*context)->audioManager, requestedChannels);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoAudioOutputDevice) device,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	if (! VuoAudioOutputDevice_areEqual(device, (*context)->device))
	{
		VuoAudioOut_removeTrigger((*context)->audioManager, requestedChannels);
		updateDevice(*context, device);
		VuoAudioOut_addTrigger((*context)->audioManager, requestedChannels);
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoAudioOutputDevice) device,
		VuoInputData(VuoList_VuoAudioSamples) sendChannels,
		VuoInputEvent({"eventBlocking":"none","data":"sendChannels"}) sendChannelsEvent,
		VuoOutputTrigger(requestedChannels, VuoReal, {"name":"Refreshed at Time"})
)
{
	if (!(*context)->triggersEnabled)
		return;

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
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoAudioOutputDevice_release((*context)->device);
	VuoRelease((*context)->audioManager);
}
