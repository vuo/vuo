/**
 * @file
 * vuo.video.step node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoVideo.h"
#include "VuoVideoFrame.h"
#include "VuoVideoOptimization.h"
#include <float.h>

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
						"exampleCompositions" : [ "StepMovie.vuo", "SaveProcessedMovie.vuo" ]
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
	VuoImage image;
	VuoReal timestamp;
	bool forward;
};

static void setMovie(struct nodeInstanceData *context, VuoText url)
{
	if(context->player != NULL)
		VuoRelease(context->player);

	context->player = VuoVideo_make(url, VuoVideoOptimization_Random);
	VuoVideo_setPlaybackRate(context->player, FORWARD_PLAYBACK_RATE);
	context->forward = true;
	if(context->image != NULL) { VuoRelease(context->image); context->image = NULL; }
	context->timestamp = VuoVideoFrame_NoTimestamp;

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
		VuoInputEvent({"name":"Go Forward","eventBlocking":"none"}) next,
		VuoInputEvent({"name":"Go Backward","eventBlocking":"none"}) previous,
		VuoInputData(VuoReal, {"default":0}) setTime,
		VuoInputEvent({"eventBlocking":"none","data":"setTime"}) setTimeEvent,
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputEvent({"eventBlocking":"wall", "data":"url", "hasPortAction":true}) urlEvent,
		VuoInputData(VuoLoopType, {"default":"none", "includeValues":["none","loop"]}) loop,
		VuoOutputData(VuoVideoFrame) videoFrame
)
{
	// VuoVideo_nextVideoFrame is special case for retain/release. Unlike getFrameAtSecond &
	// getCurrentVideoFrame, nextVideoFrame calls directly into VuoVideoDecoder to get data
	// (whereas getFrameAtSecond and getCurrentVideoFrame just return data already managed
	// by VuoVideoPlayer). That means the calling function is responsible for releasing the
	// VideoFrame at some point after sending it off.
	if((*context)->image != NULL) { VuoRelease((*context)->image); (*context)->image = NULL; }

	if (urlEvent)
		setMovie((*context), url);

	if( (*context)->player == NULL)
		return;

	if(setTimeEvent || urlEvent)
	{
		VuoReal t = VuoVideo_validateTimestamp( setTime,
												VuoVideo_getDuration((*context)->player),
												loop,
												NULL);

		if (!VuoVideo_seekToSecond( (*context)->player, t))
			VUserLog("Error: Couldn't seek.");

		if(VuoVideo_getCurrentVideoFrame((*context)->player, videoFrame))
		{
			(*context)->timestamp = videoFrame->timestamp;
			return;
		}
	}

	if(!next && !previous)
		return;

	VuoReal timestamp = (*context)->timestamp;

	if(next && !(*context)->forward)
	{
		VuoVideo_setPlaybackRate( (*context)->player, FORWARD_PLAYBACK_RATE );
		(*context)->forward = true;

		if(VuoVideo_getFrameAtSecond((*context)->player, (*context)->timestamp, videoFrame))
			timestamp = videoFrame->timestamp;
	}
	else if(previous && (*context)->forward)
	{
		VuoVideo_setPlaybackRate( (*context)->player, BACKWARD_PLAYBACK_RATE );
		(*context)->forward = false;

		if(VuoVideo_getFrameAtSecond((*context)->player, (*context)->timestamp, videoFrame))
			timestamp = videoFrame->timestamp;
	}

	while((*context)->forward ? timestamp <= (*context)->timestamp : timestamp >= (*context)->timestamp)
	{
		if((*context)->image != NULL) { VuoRelease((*context)->image); (*context)->image = NULL; }

		if( !VuoVideo_nextVideoFrame((*context)->player, videoFrame) )
		{
			if (loop == VuoLoopType_Loop)
			{
				// grab duration from VuoVideo instead of caching it on setUrl because it can change during playback as
				// more accurate values are gathered
				VuoReal duration = VuoVideo_getDuration((*context)->player);
				if (!VuoVideo_seekToSecond((*context)->player, (*context)->forward ? 0 : duration))
					VUserLog("Error: Couldn't seek.");

				if (!VuoVideo_getCurrentVideoFrame((*context)->player, videoFrame))
					VUserLog("Error: Couldn't get current video frame.");
			}
			else
			{
				if (!VuoVideo_getFrameAtSecond((*context)->player, (*context)->forward ? VuoVideo_getDuration((*context)->player) : 0, videoFrame))
					VUserLog("Error: Couldn't get video frame.");
			}

			break;
		}
		else
		{
			// Special case - see comment at start of nodeInstanceEvent
			(*context)->image = videoFrame->image;
		}

		timestamp = videoFrame->timestamp;
	}

	(*context)->timestamp = videoFrame->timestamp;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->image != NULL)
	{
		VuoRelease((*context)->image);
		(*context)->image = NULL;
	}

	if((*context)->player != NULL)
		VuoRelease((*context)->player);
}

#undef FORWARD_PLAYBACK_RATE
#undef BACKWARD_PLAYBACK_RATE
