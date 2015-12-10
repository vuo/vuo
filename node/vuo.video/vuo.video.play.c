/**
 * @file
 * vuo.video.play node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLoopType.h"
#include "VuoMovie.h"
#include <dispatch/dispatch.h>
#include "VuoUrlFetch.h"
#include "VuoVideoFrame.h"

// #define PROFILE_TIMES 1

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
					  "version" : "2.0.1",
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ "PlayMovie.vuo", "PlayMoviesAndCameraOnCube.vuo", "PlayMovieWithSound.vuo" ]
					  },
					  "dependencies" : [
						"VuoMovie",
						"VuoUrlFetch"
					  ]
				  });

#define AUDIO_ADVANCE_GET 100000
#define AUDIO_SYNC_THRESHOLD .01	// maximum diff in a/v timestamp to allow
#define AUDIO_SEC_PER_SAMPLE ((float)VuoAudioSamples_bufferSize/VuoAudioSamples_sampleRate)
#define VuoAudio_queueSize 8	// todo - get this value from VuoAudio

struct nodeInstanceData
{
	VuoMovie movie;
	bool isPlaying;

	dispatch_time_t movieStartTime;
	VuoImage lastImage;
	VuoList_VuoAudioSamples lastAudioSamples;

	VuoLoopType loop;

	VuoReal playbackRate;
	VuoReal moviePtsStart;

	double lastVideoTimestamp;
	double lastAudioTimestamp;

	dispatch_source_t movie_timer;
	dispatch_semaphore_t movie_timerCanceled;

	dispatch_source_t audio_timer;
	dispatch_semaphore_t audio_timerCanceled;

	double duration;
};

static void pauseAudio(struct nodeInstanceData *context)
{
	if( VuoMovie_containsAudio(context->movie) )
	{
		if(context->audio_timer)
		{
			dispatch_source_cancel(context->audio_timer);
			dispatch_semaphore_wait(context->audio_timerCanceled, DISPATCH_TIME_FOREVER);
			dispatch_release(context->audio_timer);
		}

		context->audio_timer = NULL;
		if(context->lastAudioSamples)
		{
			VuoRelease(context->lastAudioSamples);
			context->lastAudioSamples = NULL;
		}
	}
}

static void pauseMovie(struct nodeInstanceData *context)
{
	if (! context->movie)
		return;

	if(context->movie_timer)
	{
		dispatch_source_cancel(context->movie_timer);
		dispatch_semaphore_wait(context->movie_timerCanceled, DISPATCH_TIME_FOREVER);
		dispatch_release(context->movie_timer);
	}

	context->movie_timer = NULL;

	if(context->lastImage)
	{
		VuoRelease(context->lastImage);
		context->lastImage = NULL;
	}

	pauseAudio(context);
}


static void playNextAudioFrame(struct nodeInstanceData *context, VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples))
{
	if(context->lastAudioSamples)
	{
		// Send Audio
		if(VuoListGetCount_VuoAudioSamples(context->lastAudioSamples) > 0)
		{
			VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(context->lastAudioSamples, 1);
			decodedAudio(context->lastAudioSamples);
		}

		VuoRelease(context->lastAudioSamples);
		context->lastAudioSamples = NULL;
	}

	uint64_t cur_time = dispatch_time(DISPATCH_TIME_NOW, 0);

	if(!context->movie) return;

	context->lastAudioSamples = VuoListCreate_VuoAudioSamples();

	double frameTimestampInSecs = 0;

	bool gotFrame = VuoMovie_getNextAudioSample(context->movie, context->lastAudioSamples, &frameTimestampInSecs);
	if(gotFrame)
	{
		VuoRetain(context->lastAudioSamples);
		context->lastAudioTimestamp = frameTimestampInSecs;
	}
	else
	{
		VLog("bad");

		if(context->lastAudioSamples)
			VuoRelease(context->lastAudioSamples);

		context->lastAudioSamples = NULL;
	}

	uint64_t presentationTime = (cur_time + NSEC_PER_SEC * AUDIO_SEC_PER_SAMPLE - 100000);
	dispatch_source_set_timer(context->audio_timer, presentationTime, DISPATCH_TIME_FOREVER, NSEC_PER_SEC / 100000 );
}

static void resumeAudio(struct nodeInstanceData *context, VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples))
{
	/* audio queue */
	if( VuoMovie_containsAudio(context->movie) && context->playbackRate == 1.)
	{
		dispatch_queue_t a_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		context->audio_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, a_queue);

		// audio (plays at a steady rate)
		dispatch_source_set_timer(context->audio_timer, DISPATCH_TIME_NOW, DISPATCH_TIME_FOREVER, 0);
		dispatch_source_set_event_handler(context->audio_timer, ^{
											  playNextAudioFrame(context, decodedAudio);
										  });
		dispatch_source_set_cancel_handler(context->audio_timer, ^{
											   dispatch_semaphore_signal(context->audio_timerCanceled);
										   });
		dispatch_resume(context->audio_timer);
	}
}

static void playNextFrame(struct nodeInstanceData *context, VuoOutputTrigger(decodedVideo, VuoVideoFrame), VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples))
{
	// Display last decoded frame (if any)
	if (context->lastImage)
	{
		// VLog("Video PTS: %f", context->lastVideoTimestamp);

		decodedVideo( (VuoVideoFrame){ context->lastImage, context->lastVideoTimestamp } );
		VuoRelease(context->lastImage);
		context->lastImage = NULL;
	}

	// Decode next frame
	double presentationRelativeSeconds = 0;

	bool gotFrame = (context->playbackRate) > 0 ?
			VuoMovie_getNextVideoFrame(context->movie, &context->lastImage, &presentationRelativeSeconds) :
			VuoMovie_getPreviousVideoFrame(context->movie, &context->lastImage, &presentationRelativeSeconds);

	context->lastVideoTimestamp = presentationRelativeSeconds;	// todo - this is debug stuff

	if(gotFrame)
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

				if(context->playbackRate == 1.)
					resumeAudio(context, decodedAudio);
				else
					pauseAudio(context);

				/// @todo (https://b33p.net/kosada/node/6601)
				break;
		}
	}

	presentationRelativeSeconds = ( (presentationRelativeSeconds-context->moviePtsStart) / context->playbackRate );

	// Schedule next frame
	uint64_t presentationTime = (shouldContinue ?
									 dispatch_time(context->movieStartTime, presentationRelativeSeconds * NSEC_PER_SEC) :
									 DISPATCH_TIME_FOREVER);

	dispatch_source_set_timer(context->movie_timer, presentationTime, DISPATCH_TIME_FOREVER, NSEC_PER_SEC / 1000);
}

static void playMovie(struct nodeInstanceData *context, VuoOutputTrigger(decodedVideo, VuoVideoFrame), VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples))
{
	if (! context->movie)
		return;

	// video queue
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	context->movie_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

	// set start PTS
	context->movieStartTime = dispatch_time(DISPATCH_TIME_NOW, 0);
	context->moviePtsStart = VuoMovie_getCurrentSecond(context->movie);

	// video queue
	dispatch_source_set_timer(context->movie_timer, DISPATCH_TIME_NOW, DISPATCH_TIME_FOREVER, 0);
	dispatch_source_set_event_handler(context->movie_timer, ^{
										  playNextFrame(context, decodedVideo, decodedAudio);
									  });
	dispatch_source_set_cancel_handler(context->movie_timer, ^{
										   dispatch_semaphore_signal(context->movie_timerCanceled);
									   });
	dispatch_resume(context->movie_timer);

	resumeAudio(context, decodedAudio);
}

static void setMovie(struct nodeInstanceData *context, const char *movieURL)
{
	VuoText normalizedUrl = VuoUrl_normalize(movieURL, false);

	VuoMovie newMovie = VuoMovie_make(normalizedUrl);
	VuoRetain(normalizedUrl);
	VuoRelease(normalizedUrl);

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
		VuoInputData(VuoReal) playbackRate,
		VuoInputData(VuoLoopType) loop,
		VuoInputData(VuoReal) setTime
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->movie_timerCanceled = dispatch_semaphore_create(0);
	context->audio_timerCanceled = dispatch_semaphore_create(0);

	context->loop = loop;
	context->playbackRate = playbackRate;

	setMovie(context, url);

	if(context->movie != NULL)
	{
		VuoMovie_seekToSecond(context->movie, setTime);
	}

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(decodedVideo, VuoVideoFrame, {"eventThrottling":"drop"}),
		VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples, {"eventThrottling":"enqueue"})
)
{
	if ((*context)->isPlaying)
		playMovie(*context, decodedVideo, decodedAudio);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
		VuoInputEvent({"eventBlocking":"none","data":"url"}) urlEvent,
		VuoInputEvent({"eventBlocking":"none"}) play,
		VuoInputEvent({"eventBlocking":"none"}) pause,
		VuoInputData(VuoLoopType, {"default":"loop"}) loop,
		VuoInputData(VuoReal, {"default":1., "suggestedMin":-2., "suggestedMax":2.}) playbackRate,
		VuoInputData(VuoReal, {"default":""}) setTime,
		VuoInputEvent({"eventBlocking":"none","data":"setTime"}) setTimeEvent,

		VuoOutputTrigger(decodedVideo, VuoVideoFrame, {"eventThrottling":"drop"}),
		VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples, {"eventThrottling":"enqueue"})
)
{
	if (urlEvent)
	{
		if ((*context)->isPlaying)
			pauseMovie(*context);

		setMovie(*context, url);

		if ((*context)->isPlaying)
			playMovie(*context, decodedVideo, decodedAudio);
	}

	if((*context)->playbackRate != playbackRate)
	{
		if((*context)->isPlaying && (*context)->playbackRate != 0)
			pauseMovie(*context);

		(*context)->playbackRate = playbackRate;

		if( (*context)->isPlaying && playbackRate != 0)
			playMovie(*context, decodedVideo, decodedAudio);
	}

	if(setTimeEvent && (*context)->movie != NULL)
	{
		if ((*context)->isPlaying)
			pauseMovie((*context));

		VuoMovie_seekToSecond( (*context)->movie, setTime );

		if ((*context)->isPlaying)
			playMovie((*context), decodedVideo, decodedAudio);
	}

	(*context)->loop = loop;

	if (play && ! (*context)->isPlaying)
	{
		(*context)->isPlaying = true;
		playMovie(*context, decodedVideo, decodedAudio);
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
	if ((*context)->isPlaying)
		pauseMovie(*context);

	VuoRelease((*context)->movie);

	dispatch_release((*context)->movie_timerCanceled);
	dispatch_release((*context)->audio_timerCanceled);
}
