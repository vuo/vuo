/**
 * @file
 * vuo.audio.file.play node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioFile.h"

VuoModuleMetadata({
					  "title" : "Play Audio File",
					  "keywords" : [
						  "sound", "music", "listen",
						  "wave",
						  "aiff",
						  "mp3", "mp2",
						  "aac", "m4a", "ac3",
						  "3gp", "amr"
					  ],
					  "version" : "1.2.0",
					  "dependencies" : [
						  "VuoAudioFile"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "PlayAudioFile.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	VuoAudioFile af;
	VuoText url;
	bool triggersEnabled;
};

struct nodeInstanceData *nodeInstanceInit
(
		VuoInputData(VuoText) url,
		VuoInputData(VuoLoopType) loop,
		VuoInputData(VuoReal, {"default":""}) setTime
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->af = VuoAudioFile_make(url);
	VuoRetain(context->af);
	VuoAudioFile_setLoopType(context->af, loop);
	VuoAudioFile_setTime(context->af, setTime);

	context->url = url;
	VuoRetain(url);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(decodedChannels, VuoList_VuoAudioSamples, {"eventThrottling":"drop"}),
		VuoOutputTrigger(finishedPlayback, void)
)
{
	(*context)->triggersEnabled = true;
	VuoAudioFile_enableTriggers((*context)->af, decodedChannels, finishedPlayback);
}

void nodeInstanceTriggerUpdate( VuoInputData(VuoLoopType, {"default":"none", "includeValues":["none","loop"]}) loop,
								VuoInstanceData(struct nodeInstanceData *) context  )
{
	VuoAudioFile_setLoopType((*context)->af, loop);
}

void nodeInstanceEvent
(
		VuoInputEvent({"eventBlocking":"none"}) play,
		VuoInputEvent({"eventBlocking":"none"}) pause,
		VuoInputData(VuoReal, {"default":""}) setTime,
		VuoInputEvent({"eventBlocking":"none","data":"setTime"}) setTimeEvent,
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputEvent({"eventBlocking":"none","data":"url"}) urlEvent,
		VuoInputData(VuoLoopType, {"default":"none", "includeValues":["none","loop"]}) loop,
		VuoOutputTrigger(decodedChannels, VuoList_VuoAudioSamples, {"eventThrottling":"enqueue"}),
		VuoOutputTrigger(finishedPlayback, void),
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if (!(*context)->triggersEnabled)
		return;

	if (urlEvent || !VuoText_areEqual((*context)->url, url))
	{
		bool wasPlaying = VuoAudioFile_isPlaying((*context)->af);

		VuoAudioFile_disableTriggers((*context)->af);
		VuoRelease((*context)->af);
		(*context)->af = VuoAudioFile_make(url);
		VuoRetain((*context)->af);
		VuoAudioFile_enableTriggers((*context)->af, decodedChannels, finishedPlayback);

		VuoRelease((*context)->url);
		(*context)->url = url;
		VuoRetain((*context)->url);

		VuoAudioFile_setLoopType((*context)->af, loop);
		VuoAudioFile_setTime((*context)->af, setTime);

		if (wasPlaying)
			VuoAudioFile_play((*context)->af);
	}

	if (play)
		VuoAudioFile_play((*context)->af);

	if (pause)
		VuoAudioFile_pause((*context)->af);

	VuoAudioFile_setLoopType((*context)->af, loop);

	if (setTimeEvent)
		VuoAudioFile_setTime((*context)->af, setTime);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoAudioFile_disableTriggers((*context)->af);
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->af);
	VuoRelease((*context)->url);
}
