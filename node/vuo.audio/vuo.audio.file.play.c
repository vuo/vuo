/**
 * @file
 * vuo.audio.file.play node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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
					  "version" : "1.1.0",
					  "dependencies" : [
						  "VuoAudioFile"
					  ],
					  "node" : {
						  "isInterface" : true,
						  "exampleCompositions" : [ "PlayAudioFile.vuo", "PlayAudioFileAndLoop.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	VuoAudioFile af;
	VuoText url;
};

struct nodeInstanceData *nodeInstanceInit
(
		VuoInputData(VuoText) url,
//		VuoInputData(VuoLoopType, {"default":"loop"}) loop,
		VuoInputData(VuoReal, {"default":""}) setTime
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->af = VuoAudioFile_make(url);
	VuoRetain(context->af);
//	VuoAudioFile_setLoopType(context->af, loop);
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
	VuoAudioFile_enableTriggers((*context)->af, decodedChannels, finishedPlayback);
}

void nodeInstanceEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputEvent({"eventBlocking":"none","data":"url"}) urlEvent,
		VuoInputEvent({"eventBlocking":"none"}) play,
		VuoInputEvent({"eventBlocking":"none"}) pause,
//		VuoInputData(VuoLoopType, {"default":"loop"}) loop,
		VuoInputData(VuoReal, {"default":""}) setTime,
		VuoInputEvent({"eventBlocking":"none","data":"setTime"}) setTimeEvent,
		VuoOutputTrigger(decodedChannels, VuoList_VuoAudioSamples, {"eventThrottling":"enqueue"}),
		VuoOutputTrigger(finishedPlayback, void),
		VuoInstanceData(struct nodeInstanceData *) context
)
{
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

//		VuoAudioFile_setLoopType((*context)->af, loop);
		VuoAudioFile_setTime((*context)->af, setTime);

		if (wasPlaying)
			VuoAudioFile_play((*context)->af);
	}

	if (play)
		VuoAudioFile_play((*context)->af);

	if (pause)
		VuoAudioFile_pause((*context)->af);

//	VuoAudioFile_setLoopType((*context)->af, loop);

	if (setTimeEvent)
		VuoAudioFile_setTime((*context)->af, setTime);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoAudioFile_disableTriggers((*context)->af);
}

void nodeInstanceFini(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->af);
	VuoRelease((*context)->url);
}
