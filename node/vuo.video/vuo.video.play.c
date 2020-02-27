/**
 * @file
 * vuo.video.play node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoVideo.h"
#include "VuoVideoFrame.h"
#include "VuoAudioFrame.h"
#include "VuoLoopType.h"

VuoModuleMetadata({
					"title" : "Play Movie",
					"keywords" : [
						"record", "cinema", "animation",
						  "avi",
						  "dv", "dvc",
						  "h264", "h.264",
						  "mjpeg",
						  "mpeg", "m4v", "mp4",
						  "quicktime", "qt", "aic", "prores",
						  "video", "gif"
					],
					"version" : "2.0.3",
					"dependencies" : [
						"VuoVideo",
						"VuoFfmpegDecoder",
						"VuoVideoPlayer"
					],
					"node": {
					"exampleCompositions" : [
						"PlayMovie.vuo",
						"PlayMoviesAndCameraOnCube.vuo",
						"PlayMoviesInFolder.vuo",
						"PlayMovieWithSound.vuo"
						]
					}
				 });

struct nodeInstanceData
{
	VuoVideo player;
	VuoLoopType loop;
	VuoUrl url;

	bool isPlaying;
	VuoReal lastSoughtTime;
	VuoReal playbackRate;
	VuoVideoOptimization optimizeFor;

	bool triggersEnabled;
};

bool setVideoPath(struct nodeInstanceData* instance, VuoUrl url)
{
	if(VuoText_length(url) < 1)
		return false;

	if(instance->url != NULL)
		VuoRelease(instance->url);

	instance->url = url;
	VuoRetain(instance->url);

	if(instance->player != NULL)
	{
		VuoRelease(instance->player);
		instance->player = NULL;
	}

	VuoVideoOptimization opt = instance->optimizeFor;

	if(opt == VuoVideoOptimization_Auto)
		opt = instance->playbackRate < 0 || instance->loop == VuoLoopType_Mirror ? VuoVideoOptimization_Random : VuoVideoOptimization_Forward;

	instance->player = VuoVideo_make(url, opt);

	if(instance->player != NULL)
	{
		VuoRetain(instance->player);
		VuoVideo_setPlaybackLooping(instance->player, instance->loop);
		VuoVideo_setPlaybackRate(instance->player, instance->playbackRate);

		return true;
	}

	return false;
}

void vuo_video_play_resume(struct nodeInstanceData *instance, void (*decodedVideo)(VuoVideoFrame), void (*decodedAudio)(VuoList_VuoAudioSamples), void (*finishedPlayback)(void))
{
	VuoVideo_setVideoDelegate(instance->player, decodedVideo);
	VuoVideo_setAudioDelegate(instance->player, decodedAudio);
	VuoVideo_setPlaybackFinishedDelegate(instance->player, finishedPlayback);

	if (instance->isPlaying)
		VuoVideo_play(instance->player);
}

/**
 * Validates the specified time, seeks to the specified time, and sets the playback rate (in case mirror loop mode is or was used).
 */
static void vuo_video_play_setTime(struct nodeInstanceData *instance, VuoReal time)
{
	VuoReal duration = VuoVideo_getDuration(instance->player);
	int direction;
	VuoReal sought = VuoVideo_validateTimestamp(time, duration, instance->loop, &direction);
	instance->lastSoughtTime = sought;
	if (!VuoVideo_seekToSecond(instance->player, sought))
		VUserLog("Error: Couldn't seek.");

	VuoVideo_setPlaybackRate(instance->player, instance->playbackRate * direction);
}

struct nodeInstanceData* nodeInstanceInit(
	VuoInputData(VuoText) url,
	VuoInputData(VuoLoopType) loop,
	VuoInputData(VuoReal) playbackRate,
	VuoInputData(VuoReal) setTime,
	VuoInputData(VuoVideoOptimization) optimization
	)
{
	struct nodeInstanceData* instance = (struct nodeInstanceData*) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->isPlaying = false;
	instance->playbackRate = playbackRate;
	instance->optimizeFor = optimization;
	instance->loop = loop;

	setVideoPath(instance, url);
	vuo_video_play_setTime(instance, setTime);

	return instance;
}

void nodeInstanceTriggerStart(
	VuoInstanceData(struct nodeInstanceData*) instance,
	VuoOutputTrigger(decodedVideo, VuoVideoFrame),
	VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples),
	VuoOutputTrigger(finishedPlayback, void)
	)
{
	(*instance)->triggersEnabled = true;

	vuo_video_play_resume(*instance, decodedVideo, decodedAudio, finishedPlayback);
}

void nodeInstanceTriggerStop(
	VuoInstanceData(struct nodeInstanceData*) instance
	)
{
	if((*instance)->isPlaying)
	{
		VuoVideo_pause((*instance)->player);

		VuoVideo_setVideoDelegate((*instance)->player, NULL);
		VuoVideo_setAudioDelegate((*instance)->player, NULL);
		VuoVideo_setPlaybackFinishedDelegate((*instance)->player, NULL);
	}

	(*instance)->triggersEnabled = false;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData*) instance,
	VuoInputEvent({"eventBlocking":"none"}) play,
	VuoInputEvent({"eventBlocking":"none"}) pause,
	VuoInputData(VuoReal, {"default":""}) setTime,
	VuoInputEvent({"eventBlocking":"none","data":"setTime"}) setTimeEvent,
	VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
	VuoInputEvent({"eventBlocking":"none","data":"url"}) urlEvent,
	VuoInputData(VuoLoopType, {"default":"loop"}) loop,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":-2, "suggestedMax":2, "suggestedStep":0.1}) playbackRate,
	VuoInputData(VuoVideoOptimization, {"default":"auto"}) optimization,
	VuoInputEvent({"eventBlocking":"none", "data":"optimization"}) optimizationChanged,
	VuoOutputTrigger(decodedVideo, VuoVideoFrame, {"eventThrottling":"drop"}),
	VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples),
	VuoOutputTrigger(finishedPlayback, void)
	)
{
	if (!(*instance)->triggersEnabled)
		return;

	if(urlEvent)
	{
		if(setVideoPath(*instance, url))
		{
			// When changing the URL, seek before resuming playback, so we don't miss frames.
			vuo_video_play_setTime(*instance, setTime);

			vuo_video_play_resume(*instance, decodedVideo, decodedAudio, finishedPlayback);
		}
	}

	if(optimizationChanged)
	{
		// get current time, then reload the video and sync back up
		VuoReal timestamp = VuoVideo_getLastDecodedVideoTimestamp((*instance)->player);

		(*instance)->optimizeFor = optimization;

		setVideoPath(*instance, url);

		if (!VuoVideo_seekToSecond((*instance)->player, timestamp))
			VUserLog("Error: Couldn't seek.");

		vuo_video_play_resume(*instance, decodedVideo, decodedAudio, finishedPlayback);
	}

	(*instance)->loop = loop;

	if (setTimeEvent && !urlEvent)
	{
		if((*instance)->player != NULL)
		{
			vuo_video_play_setTime(*instance, setTime);

			// if loop mode is none & playback has run it's course the player
			// and node play states will be out of sync
			if( (*instance)->isPlaying && !VuoVideo_isPlaying((*instance)->player) )
				VuoVideo_play((*instance)->player);
		}
	}

	if(play && (!(*instance)->isPlaying || !VuoVideo_isPlaying((*instance)->player)))
	{
		(*instance)->isPlaying = true;
		VuoVideo_play((*instance)->player);
	}

	if(pause && (*instance)->isPlaying)
	{
		(*instance)->isPlaying = false;
		VuoVideo_pause((*instance)->player);
	}

	VuoVideo_setPlaybackLooping((*instance)->player, loop);

	if( !VuoReal_areEqual((*instance)->playbackRate, playbackRate) )
	{
		(*instance)->playbackRate = playbackRate;
		VuoVideo_setPlaybackRate((*instance)->player, playbackRate);
	}
}

void nodeInstanceTriggerUpdate(
	VuoInstanceData(struct nodeInstanceData*) instance,
	VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
	VuoInputData(VuoLoopType, {"default":"loop"}) loop,
	VuoInputData(VuoReal, {"default":1}) playbackRate,
	VuoInputData(VuoReal) setTime,
	VuoInputData(VuoVideoOptimization, {"default":"auto"}) optimization,
	VuoOutputTrigger(decodedVideo, VuoVideoFrame, {"eventThrottling":"drop"}),
	VuoOutputTrigger(decodedAudio, VuoList_VuoAudioSamples),
	VuoOutputTrigger(finishedPlayback, void)
	)
{
	if(optimization != (*instance)->optimizeFor)
	{
		// get current time, then reload the video and sync back up
		VuoReal timestamp = VuoVideo_getLastDecodedVideoTimestamp((*instance)->player);

		(*instance)->optimizeFor = optimization;

		setVideoPath(*instance, url);

		if (!VuoVideo_seekToSecond((*instance)->player, timestamp))
			VUserLog("Error: Couldn't seek.");

		vuo_video_play_resume(*instance, decodedVideo, decodedAudio, finishedPlayback);
	}

	bool urlChanged = false;
	if(VuoText_length(url) > 2 && !VuoText_areEqual(url, (*instance)->url))
	{
		urlChanged = true;
		if( setVideoPath(*instance, url) )
		{
			// When changing the URL, seek before resuming playback, so we don't miss frames.
			vuo_video_play_setTime(*instance, setTime);

			vuo_video_play_resume(*instance, decodedVideo, decodedAudio, finishedPlayback);
		}
	}

	(*instance)->loop = loop;
	VuoVideo_setPlaybackLooping((*instance)->player, loop);

	if( !VuoReal_areEqual((*instance)->playbackRate, playbackRate) )
	{
		(*instance)->playbackRate = playbackRate;
		VuoVideo_setPlaybackRate((*instance)->player, playbackRate);
	}

	if (!VuoReal_areEqual((*instance)->lastSoughtTime, setTime) && !urlChanged)
		vuo_video_play_setTime(*instance, setTime);
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData*) instance
)
{
	if((*instance)->player != NULL)
	{
		VuoRelease( (*instance)->player );
		(*instance)->player = NULL;
	}

	if((*instance)->url != NULL)
		VuoRelease((*instance)->url);
}
