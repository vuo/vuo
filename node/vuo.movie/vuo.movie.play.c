/**
 * @file
 * vuo.movie.play node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
						  "exampleCompositions" : [
							  "PlayMovie.vuo",
							  "PlayMoviesOnCube.vuo",
							  "PlayMovieWithButton.vuo",
							  "SimulateFilmProjector.vuo"
						  ]
					  },
					  "dependencies" : [
						"VuoMovie"
					  ]
				  });


struct nodeInstanceData
{
	VuoMovie movie;
	bool isPlaying;

	dispatch_time_t movieStartTime;
	VuoImage lastImage;

	VuoLoopType loop;

	VuoReal playbackRate;
	VuoReal moviePtsStart;

	dispatch_source_t timer;
	dispatch_semaphore_t timerCanceled;

	double duration;
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

	bool gotFrame = (context->playbackRate) > 0 ?
			VuoMovie_getNextFrame(context->movie, &context->lastImage, &presentationRelativeSeconds) :
			VuoMovie_getPreviousFrame(context->movie, &context->lastImage, &presentationRelativeSeconds);

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

				presentationRelativeSeconds = context->playbackRate > 0 ? 0 : context->duration;
				VuoMovie_seekToSecond(context->movie, presentationRelativeSeconds);
				context->movieStartTime = dispatch_time(DISPATCH_TIME_NOW, 0);
				context->moviePtsStart = VuoMovie_getCurrentSecond(context->movie);

				break;

			case VuoLoopType_Mirror:

				context->playbackRate = -context->playbackRate;
				context->movieStartTime = dispatch_time(DISPATCH_TIME_NOW, 0);
				context->moviePtsStart = VuoMovie_getCurrentSecond(context->movie);

				presentationRelativeSeconds = context->playbackRate > 0 ? 0 : context->duration;

				/// @todo (https://b33p.net/kosada/node/6601)
				break;
		}
	}

	presentationRelativeSeconds = ( (presentationRelativeSeconds-context->moviePtsStart) / context->playbackRate );

	// Schedule next frame
	uint64_t presentationTime = (shouldContinue ?
									 dispatch_time(context->movieStartTime, presentationRelativeSeconds * NSEC_PER_SEC) :
									 DISPATCH_TIME_FOREVER);

	dispatch_source_set_timer(context->timer, presentationTime, DISPATCH_TIME_FOREVER, NSEC_PER_SEC / 1000);
}

static void playMovie(struct nodeInstanceData *context, VuoOutputTrigger(decodedImage, VuoImage))
{
	if (! context->movie)
		return;

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	context->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

	context->movieStartTime = dispatch_time(DISPATCH_TIME_NOW, 0);//, - ((VuoMovie_getCurrentSecond(context->movie) * (1./context->playbackRate)) * NSEC_PER_SEC));
	context->moviePtsStart = VuoMovie_getCurrentSecond(context->movie);

	dispatch_source_set_timer(context->timer, DISPATCH_TIME_NOW, DISPATCH_TIME_FOREVER, 0);
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
		VuoInputData(VuoText) movieURL,
		VuoInputData(VuoReal) playbackRate,
		VuoInputData(VuoLoopType) loop,
		VuoInputData(VuoReal) setTime
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->timerCanceled = dispatch_semaphore_create(0);
	context->loop = loop;
	context->playbackRate = playbackRate;
	setMovie(context, movieURL);

	if(context->movie != NULL)
		VuoMovie_seekToSecond(context->movie, setTime);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(decodedImage, VuoImage, VuoPortEventThrottling_Drop)
)
{
	if ((*context)->isPlaying)
		playMovie(*context, decodedImage);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoText, {"default":""}) movieURL,
		VuoInputEvent(VuoPortEventBlocking_None, movieURL) movieURLEvent,
		VuoInputEvent(VuoPortEventBlocking_None,) play,
		VuoInputEvent(VuoPortEventBlocking_None,) pause,
		VuoInputData(VuoLoopType, {"default":"loop"}) loop,
		VuoInputData(VuoReal, {"default":1., "suggestedMin":-2., "suggestedMax":2.}) playbackRate,   /// @todo (https://b33p.net/kosada/node/6596, https://b33p.net/kosada/node/6597)

		VuoInputData(VuoReal, {"default":""}) setTime,
		VuoInputEvent(VuoPortEventBlocking_None, setTime) setTimeEvent,

		VuoOutputTrigger(decodedImage, VuoImage, VuoPortEventThrottling_Drop)
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

	if((*context)->playbackRate != playbackRate)
	{
		if((*context)->isPlaying && (*context)->playbackRate != 0)
			pauseMovie(*context);

		(*context)->playbackRate = playbackRate;

		if( (*context)->isPlaying && playbackRate != 0)
			playMovie(*context, decodedImage);
	}

	if(setTimeEvent && (*context)->movie != NULL)
	{
		if ((*context)->isPlaying)
			pauseMovie((*context));

		VuoMovie_seekToSecond( (*context)->movie, setTime );

		if ((*context)->isPlaying)
			playMovie((*context), decodedImage);
	}

	(*context)->loop = loop;

	if (play && ! (*context)->isPlaying)
	{
		(*context)->isPlaying = true;
		playMovie(*context, decodedImage);
	}

	if( pause && (*context)->isPlaying)
	{
		(*context)->isPlaying = false;
		pauseMovie(*context);
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
