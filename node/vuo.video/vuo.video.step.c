/**
 * @file
 * vuo.video.step node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoVideo.h"
#include "VuoVideoFrame.h"
#include "VuoVideoOptimization.h"

VuoModuleMetadata({
					  "title" : "Step Movie Frame",
					  "keywords" : [
						  "animation",
						  "avi",
						  "dv", "dvc",
						  "h264", "h.264",
						  "mjpeg",
						  "mpeg", "m4v", "mp4",
						  "quicktime", "qt", "aic", "prores",
						  "video",
					  ],
					  "version" : "1.0.0",
					  "node": {
					  },
					  "dependencies" : [
						"VuoVideo",
						"VuoUrl",
						"VuoFfmpegDecoder"
					  ]
				  });

/// Define forwards playback rate as non-1 so that FFMPEG doesn't bother decoding audio
#define FORWARD_PLAYBACK_RATE .9

/// Backwards playback rate
#define BACKWARD_PLAYBACK_RATE -1

struct nodeInstanceData
{
	VuoVideo player;
	bool forward;
};

static void setMovie(struct nodeInstanceData *context, VuoText url)
{
	if(context->player != NULL)
		VuoRelease(context->player);

	context->player = VuoVideo_make(url, VuoVideoOptimization_Random);
	context->forward = true;

	if(context->player != NULL)
		VuoRetain(context->player);
}

struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoText) url
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->player = NULL;
	setMovie(context, url);

	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputEvent({"eventBlocking":"wall", "data":"url", "hasPortAction":true}) urlEvent,
		VuoInputEvent({"eventBlocking":"none"}) next,
		VuoInputEvent({"eventBlocking":"none"}) previous,
		VuoInputData(VuoBoolean, {"default":false}) loop,
		VuoInputData(VuoReal, {"default":0}) setTime,
		VuoInputEvent({"eventBlocking":"none","data":"setTime"}) setTimeEvent,
		VuoOutputData(VuoVideoFrame) videoFrame
)
{
	if (urlEvent)
		setMovie((*context), url);

	if( (*context)->player == NULL)
		return;

	if(setTimeEvent)
	{
		VuoVideo_seekToSecond( (*context)->player, VuoVideo_validateTimestamp(setTime, VuoVideo_getDuration((*context)->player), loop ? VuoLoopType_Loop : VuoLoopType_None) );
		if(VuoVideo_getCurrentVideoFrame((*context)->player, videoFrame))
			return;
	}

	if(!next && !previous)
		return;

	if(next && !(*context)->forward)
	{
		VuoVideo_setPlaybackRate( (*context)->player, FORWARD_PLAYBACK_RATE );
		(*context)->forward = true;
	}
	else if(previous && (*context)->forward)
	{
		VuoVideo_setPlaybackRate( (*context)->player, BACKWARD_PLAYBACK_RATE );
		(*context)->forward = false;
	}

	if( !VuoVideo_nextVideoFrame((*context)->player, videoFrame) )
	{
		if(loop)
		{
			// grab duration from VuoVideo instead of caching it on setUrl because they it change during playback
			VuoReal duration = VuoVideo_getDuration((*context)->player);
			VuoVideo_seekToSecond((*context)->player, (*context)->forward ? 0 : duration);
			VuoVideo_getCurrentVideoFrame((*context)->player, videoFrame);
		}
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if((*context)->player != NULL)
		VuoRelease((*context)->player);
}

#undef FORWARD_PLAYBACK_RATE
#undef BACKWARD_PLAYBACK_RATE
