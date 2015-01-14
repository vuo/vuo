/**
 * @file
 * vuo.movie.play node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLoopType.h"
#include "VuoMovie.h"
#include <dispatch/dispatch.h>

VuoModuleMetadata({
					  "title" : "Play Movie",
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
						  "isInterface" : true,
						  "exampleCompositions" : [ "PlayMovie.vuo" ]
					  },
					  "dependencies" : [
						"VuoMovie"
					  ]
				  });


struct nodeInstanceData
{
	VuoMovie *movie;
	bool isPlaying;

	dispatch_time_t movieStartTime;
	VuoImage lastImage;

	VuoLoopType loop;

	dispatch_source_t timer;
	dispatch_semaphore_t timerCanceled;
};


static void pauseMovie(struct nodeInstanceData *context)
{
	if (! context->movie)
		return;

	dispatch_source_cancel(context->timer);
	dispatch_semaphore_wait(context->timerCanceled, DISPATCH_TIME_FOREVER);
	dispatch_release(context->timer);
	context->timer = NULL;

	VuoRelease(context->lastImage);
	context->lastImage = NULL;
}

static void playNextFrame(struct nodeInstanceData *context, VuoOutputTrigger(decodedImage, VuoImage))
{
	// Display last decoded frame (if any)
	if (context->lastImage)
	{
		decodedImage(context->lastImage);
		VuoRelease(context->lastImage);
		context->lastImage = NULL;
	}

	// Decode next frame
	double presentationRelativeSeconds = 0;
	bool gotFrame = VuoMovie_getNextFrame(context->movie, &context->lastImage, &presentationRelativeSeconds);

	VuoRetain(context->lastImage);

	bool shouldContinue = true;
	if (! gotFrame)
	{
		switch(context->loop)
		{
			case VuoLoopType_None:
				shouldContinue = false;
				break;

			case VuoLoopType_Loop:
				VuoMovie_seekToSecond( context->movie, 0.);
				context->movieStartTime = dispatch_time(DISPATCH_TIME_NOW, 0);
				presentationRelativeSeconds = 0;
				break;

			case VuoLoopType_Mirror:
				/// @todo (https://b33p.net/kosada/node/6601)
				break;
		}
	}

	// Schedule next frame
	uint64_t presentationTime = (shouldContinue ?
									 dispatch_time(context->movieStartTime, presentationRelativeSeconds * NSEC_PER_SEC) :
									 DISPATCH_TIME_FOREVER);

	dispatch_source_set_timer(context->timer, presentationTime, 1, NSEC_PER_SEC / 1000);
}

static void playMovie(struct nodeInstanceData *context, VuoOutputTrigger(decodedImage, VuoImage))
{
	if (! context->movie)
		return;

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	context->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

	context->movieStartTime = dispatch_time(DISPATCH_TIME_NOW, 0);
//	context->debugFrameCount = 0;

	dispatch_source_set_timer(context->timer, DISPATCH_TIME_NOW, 1, 0);
	dispatch_source_set_event_handler(context->timer, ^{
										  playNextFrame(context, decodedImage);
									  });
	dispatch_source_set_cancel_handler(context->timer, ^{
										   dispatch_semaphore_signal(context->timerCanceled);
									   });
	dispatch_resume(context->timer);
}

static void setMovie(struct nodeInstanceData *context, const char *movieURL)
{
	VuoMovie *newMovie = VuoMovie_make(movieURL);

	// If VuoMovie_make fails to initialize properly, it cleans up after itself.
	// No need to call VuoMovie_free()
	if(newMovie == NULL)
		return;

	VuoRetain(newMovie);
	VuoRelease(context->movie);
	context->movie = newMovie;
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoText) movieURL,
		VuoInputData(VuoLoopType) loop
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->timerCanceled = dispatch_semaphore_create(0);
	context->loop = loop;
	setMovie(context, movieURL);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(decodedImage, VuoImage)
)
{
	if ((*context)->isPlaying)
		playMovie(*context, decodedImage);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoText, {"default":""}) movieURL,
		VuoInputEvent(VuoPortEventBlocking_Wall, movieURL) movieURLEvent,
		VuoInputEvent(VuoPortEventBlocking_None,) play,
		VuoInputData(VuoLoopType, {"default":"loop"}) loop,
//		VuoInputData(VuoReal, {"default":1., "suggestedMin":-2., "suggestedMax":2.}) playbackRate,   /// @todo (https://b33p.net/kosada/node/6596, https://b33p.net/kosada/node/6597)
		VuoOutputTrigger(decodedImage, VuoImage)
)
{
	if (movieURLEvent)
	{
		if ((*context)->isPlaying)
			pauseMovie(*context);

		setMovie(*context, movieURL);

		if ((*context)->isPlaying)
			playMovie(*context, decodedImage);
	}

	(*context)->loop = loop;

	if (play && ! (*context)->isPlaying)
	{
		(*context)->isPlaying = true;
		playMovie(*context, decodedImage);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->isPlaying)
		pauseMovie(*context);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->movie);
	dispatch_release((*context)->timerCanceled);
}
