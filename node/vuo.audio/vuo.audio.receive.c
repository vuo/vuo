/**
 * @file
 * vuo.audio.receive node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudio.h"
#include "VuoAudioSamples.h"

VuoModuleMetadata({
					  "title" : "Receive Live Audio",
					  "keywords" : [ "sound", "microphone", "music", "listen", "device" ],
					  "version" : "1.0.2",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "exampleCompositions" : [ "ShowLiveAudioWaveform.vuo", "VisualizeLoudness.vuo" ],
					  }
				 });


struct nodeInstanceData
{
	VuoAudioInputDevice device;
	VuoAudioIn audioManager;
	bool triggersEnabled;
};

static void updateDevice(struct nodeInstanceData *context, VuoAudioInputDevice newDevice)
{
	VuoAudioInputDevice_release(context->device);
	context->device = newDevice;
	VuoAudioInputDevice_retain(context->device);

	VuoAudioIn_disuseShared(context->audioManager);
	context->audioManager = VuoAudioIn_useShared(newDevice);
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
	(*context)->triggersEnabled = true;
	VuoAudioIn_addTrigger((*context)->audioManager, receivedChannels);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoAudioInputDevice) device,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
)
{
	if (! VuoAudioInputDevice_areEqual(device, (*context)->device))
	{
		VuoAudioIn_removeTrigger((*context)->audioManager, receivedChannels);
		updateDevice(*context, device);
		VuoAudioIn_addTrigger((*context)->audioManager, receivedChannels);
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoAudioInputDevice) device,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples, {"eventThrottling":"enqueue"})
)
{
	if (!(*context)->triggersEnabled)
		return;

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
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoAudioInputDevice_release((*context)->device);
	VuoAudioIn_disuseShared((*context)->audioManager);
}
