/**
 * @file
 * vuo.video.decodeImage node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMovie.h"
#include "VuoVideoFrame.h"

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
					  "version" : "2.0.0",
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ "SkimMovie.vuo" ]
					  },
					  "dependencies" : [
						"VuoMovie"
					  ]
				  });


struct nodeInstanceData
{
	VuoMovie movie;
	VuoReal duration;
};

static void setMovie(struct nodeInstanceData *context, const char *movieURL)
{
	VuoMovie newMovie = VuoMovie_make(movieURL);

	// If VuoMovie_make fails to initialize properly, it cleans up after itself.
	// No need to call VuoMovie_free()
	if(newMovie == NULL)
		return;

	VuoRetain(newMovie);
	VuoRelease(context->movie);
	context->movie = newMovie;
	context->duration = VuoMovie_getDuration(context->movie);
}

struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoText) url,
		VuoInputData(VuoReal) frameTime
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	setMovie(context, url);

	if(context->movie != NULL)
		VuoMovie_seekToSecond(context->movie, frameTime);

	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputEvent({"eventBlocking":"wall","data":"url","hasPortAction":true}) urlEvent,
		VuoInputData(VuoReal, {"suggestedMin":0.}) frameTime,
		VuoOutputData(VuoVideoFrame) videoFrame
)
{
	if (urlEvent)
	{
		setMovie((*context), url);
	}

	if((*context)->movie != NULL)
	{
		VuoMovie_seekToSecond((*context)->movie, frameTime);
		double nextFrameTime;
		VuoImage img;

		// will fail if frameTime is out of bounds
		if( VuoMovie_getNextVideoFrame((*context)->movie, &img, &nextFrameTime) )
		{
			// getCurrentSecond() because it may not be the same as frameTime.
			*videoFrame = (VuoVideoFrame){ img, VuoMovie_getCurrentSecond((*context)->movie) };
		}
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->movie);
}
