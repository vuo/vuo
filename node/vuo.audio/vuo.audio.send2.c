/**
 * @file
 * vuo.audio.send node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudio.h"

VuoModuleMetadata({
					  "title" : "Send Live Audio",
					  "keywords" : [ "sound", "play", "speaker", "music", "samples", "device" ],
					  "version" : "2.0.1",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "exampleCompositions" : [ "PlayAudioFile.vuo", "PlayAudioWave.vuo", "PanAudio.vuo", "GenerateParametricAudio.vuo" ],
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

	VuoAudioOut_disuseShared(context->audioManager);
	context->audioManager = VuoAudioOut_useShared(newDevice);
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

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoAudioOutputDevice) device,
		VuoInputData(VuoList_VuoAudioSamples) sendChannels,
		VuoInputEvent({"eventBlocking":"none","data":"sendChannels"}) sendChannelsEvent
)
{
	if (! VuoAudioOutputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device);

	if (sendChannelsEvent)
		VuoAudioOut_sendChannels((*context)->audioManager, sendChannels, context);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoAudioOutputDevice_release((*context)->device);
	VuoAudioOut_disuseShared((*context)->audioManager);
}
