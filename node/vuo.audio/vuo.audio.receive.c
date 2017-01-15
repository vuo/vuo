/**
 * @file
 * vuo.audio.receive node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"
#include "VuoAudioSamples.h"

VuoModuleMetadata({
					  "title" : "Receive Live Audio",
					  "keywords" : [ "sound", "microphone", "music", "listen", "device" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "exampleCompositions" : [ "ShowLiveAudioWaveform.vuo", "VisualizeLoudness.vuo" ],
						  "isInterface" : true
					  }
				 });


struct nodeInstanceData
{
	VuoAudioInputDevice device;
	VuoAudioIn audioManager;
};

static void updateDevice(struct nodeInstanceData *context, VuoAudioInputDevice newDevice)
{
	VuoAudioInputDevice_release(context->device);
	context->device = newDevice;
	VuoAudioInputDevice_retain(context->device);

	VuoRelease(context->audioManager);
	context->audioManager = VuoAudioIn_getShared(newDevice);
	VuoRetain(context->audioManager);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoAudioInputDevice) device
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
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
)
{
	VuoAudioIn_addTrigger((*context)->audioManager, receivedChannels);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoAudioInputDevice) device,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples, {"eventThrottling":"drop"})
)
{
	if (! VuoAudioInputDevice_areEqual(device, (*context)->device))
	{
		VuoAudioIn_removeTrigger((*context)->audioManager, receivedChannels);
		updateDevice(*context, device);
		VuoAudioIn_addTrigger((*context)->audioManager, receivedChannels);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
)
{
	VuoAudioIn_removeTrigger((*context)->audioManager, receivedChannels);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoAudioInputDevice_release((*context)->device);
	VuoRelease((*context)->audioManager);
}
