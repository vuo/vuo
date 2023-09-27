/**
 * @file
 * VuoVideo implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoVideo.h"
#include "VuoVideoPlayer.hh"

extern "C"
{

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoVideo",
					 "dependencies" : [
						"VuoVideoPlayer",
						"VuoImage",
						"VuoAudioSamples",
						"VuoUrl",
						"VuoVideoFrame",
						"VuoReal",
						"VuoList_VuoAudioSamples",
						"VuoList_VuoReal"
					 ]
				 });
#endif
}

/**
 * Destroy an instance of VuoVideoPlayer.
 */
void VuoVideo_free(void* player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return;
	obj->Destroy();
}

/**
 * Creates a video player for `path`.
 */
VuoVideo VuoVideo_make(VuoUrl path, VuoVideoOptimization optimization)
{
	if (VuoText_isEmpty(path))
		return NULL;

	VuoVideoPlayer* player = VuoVideoPlayer::Create(path, optimization);

	if(player != NULL)
		VuoRegister(player, VuoVideo_free);

	return player;
}

/// Set the delgate to be called when a new video frame is available.  Video frames
/// will be send to delegates at the appropriate timestamp automatically when playing.
void VuoVideo_setVideoDelegate(VuoVideo player, void(*delegate)(VuoVideoFrame))
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return;
	obj->SetVideoDelegate(delegate);
}


/// Set the delgate to be called when a new video frame is available.  Video frames
/// will be send to delegates at the appropriate timestamp automatically when playing.
void VuoVideo_setAudioDelegate(VuoVideo player, void(*delegate)(VuoList_VuoAudioSamples))
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return;
	obj->SetAudioDelegate(delegate);
}

void VuoVideo_setPlaybackFinishedDelegate(VuoVideo player, void(*delegate)(void))
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return;
	obj->SetFinishedDelegate(delegate);
}

/**
 * Return the full path to the currently playing video.
 */
const char* VuoVideo_getPath(VuoVideo player);

/**
 * Begin playback of video. Frames will be sent to delegates as set by VuoVideo_SetVideoDelegate and VuoVideo_SetAudioDelegate.
 */
void VuoVideo_play(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return;
	obj->Play();
}

/**
 * Stop playback of video.
 */
void VuoVideo_pause(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return;
	obj->Pause();
}

bool VuoVideo_seekToSecond(VuoVideo player, VuoReal second)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return false;

	return obj->Seek(second);
}

double VuoVideo_getDuration(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return 0;
	return obj->GetDuration();
}

void VuoVideo_setPlaybackLooping(VuoVideo player, VuoLoopType loop)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return;

	obj->loop = loop;
}

void VuoVideo_setPlaybackRate(VuoVideo player, VuoReal rate)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*) player;
	if(!obj) return;

	obj->SetPlaybackRate(rate);
}

double VuoVideo_getLastDecodedVideoTimestamp(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*) player;
	if(obj == NULL) return 0.;
	return obj->GetCurrentTimestamp();
}

bool VuoVideo_isPlaying(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*) player;
	if(obj == NULL) return 0.;
	return obj->IsPlaying();
}

double VuoVideo_getLastFrameDelta(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*) player;
	if(obj == NULL) return 0.;
	return obj->GetLastFrameDelta();
}

bool VuoVideo_getFrameAtSecond(VuoVideo player, VuoReal second, VuoVideoFrame* videoFrame)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;

	if(obj == NULL)
		return false;

	return obj->GetVideoFrameAtSecond(second, videoFrame);
}

bool VuoVideo_nextVideoFrame(VuoVideo player, VuoVideoFrame* videoFrame)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*) player;
	if(obj == NULL) return false;
	return obj->NextVideoFrame(videoFrame);
}

bool VuoVideo_nextAudioFrame(VuoVideo player, VuoAudioFrame* audioFrame)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*) player;
	if(obj == NULL) return false;
	return obj->NextAudioFrame(audioFrame);
}

unsigned int VuoVideo_getAudioChannels(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL)
		return 0;

	return obj->GetAudioChannels();
}

bool VuoVideo_isReady(VuoVideo player)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return false;
	return obj->IsReady();
}

bool VuoVideo_getCurrentVideoFrame(VuoVideo player, VuoVideoFrame* videoFrame)
{
	VuoVideoPlayer* obj = (VuoVideoPlayer*)player;
	if(obj == NULL) return false;
	return obj->GetCurrentVideoFrame(videoFrame);
}

VuoReal VuoVideo_validateTimestamp(VuoReal frameTime, VuoReal duration, VuoLoopType loop, int *outputDirection)
{
	if (outputDirection)
	{
		if (loop == VuoLoopType_Mirror && ((int)floor(fabs(frameTime) / duration) % 2))
			*outputDirection = -1;
		else
			*outputDirection = 1;
	}

	bool reverse = frameTime < 0;
	VuoReal timestamp = frameTime;

	if(loop != VuoLoopType_None && (frameTime < 0 || frameTime > duration))
	{
		timestamp = fabs(frameTime);
		float mod = fmod(timestamp, duration);

		if(loop == VuoLoopType_Loop)
			timestamp = reverse ? duration - mod : mod;
		else if(loop == VuoLoopType_Mirror)
			timestamp = ((int)floor(timestamp / duration) % 2) ? duration - mod : mod;
	}

	return timestamp;
}
