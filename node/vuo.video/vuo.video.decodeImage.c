/**
 * @file
 * vuo.video.decodeImage node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoVideo.h"
#include "VuoVideoFrame.h"
#include "VuoVideoOptimization.h"

VuoModuleMetadata({
					  "title" : "Decode Movie Image",
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
					  "version" : "2.2.1",
					  "node": {
						  "exampleCompositions" : [ "SkimMovie.vuo" ]
					  },
					  "dependencies" : [
						"VuoVideo",
						"VuoUrl",
						"VuoFfmpegDecoder"
					  ]
				  });


struct nodeInstanceData
{
	VuoVideo player;
	VuoReal duration;
};

static void setMovie(struct nodeInstanceData *context, VuoText url)
{
	if(context->player != NULL)
		VuoRelease(context->player);

	context->player = VuoVideo_make(url, VuoVideoOptimization_Random);
	VuoVideo_setPlaybackRate(context->player, 0);

	if(context->player != NULL)
	{
		VuoRetain(context->player);
		context->duration = VuoVideo_getDuration(context->player);
	}
}

struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoText) url,
		VuoInputData(VuoReal) frameTime
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
		VuoInputEvent({"eventBlocking":"wall","data":"url","hasPortAction":true}) urlEvent,
		VuoInputData(VuoReal, {"suggestedMin":0.}) frameTime,
		VuoInputData(VuoLoopType, {"default":"loop"}) loop,
		VuoOutputData(VuoVideoFrame) videoFrame
)
{
	if (urlEvent)
	{
		setMovie((*context), url);
	}

	if((*context)->player != NULL)
	{
		double timestamp = VuoVideo_validateTimestamp(frameTime, (*context)->duration, loop, NULL);
		if (!VuoVideo_getFrameAtSecond((*context)->player, timestamp, videoFrame))
		{
			VUserLog("Error: Couldn't get frame.");
			*videoFrame = (VuoVideoFrame){NULL, 0, 0};
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
