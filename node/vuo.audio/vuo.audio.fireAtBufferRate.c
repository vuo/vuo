/**
 * @file
 * vuo.audio.fireAtBufferRate node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					  "title" : "Fire at Audio Rate",
					  "keywords" : [ "sound", "device", "buffers", "samples", "periodically" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoAudio"
					  ],
					  "node": {
						  "exampleCompositions" : [ "PlayAudioWave.vuo" ],
					  }
				 });

struct nodeInstanceData
{
	VuoAudioOutputDevice device;
	VuoAudioIn audioManager;
	bool triggersEnabled;
};

static void updateDevice(struct nodeInstanceData *context)
{
	VuoAudioOutputDevice defaultDevice = VuoAudioOutputDevice_makeFromJson(NULL);

	VuoAudioOutputDevice_release(context->device);
	context->device = defaultDevice;
	VuoAudioOutputDevice_retain(context->device);

	VuoRelease(context->audioManager);
	context->audioManager = VuoAudioOut_getShared(defaultDevice);
	VuoRetain(context->audioManager);
}


struct nodeInstanceData * nodeInstanceInit
(
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	updateDevice(context);
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

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(requestedChannels, VuoReal, {"name":"Refreshed at Time"})
)
{
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
