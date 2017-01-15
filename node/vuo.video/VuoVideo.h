/**
 * @file
 * VuoVideo interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include "VuoImage.h"
#include "VuoAudioSamples.h"
#include "VuoList_VuoAudioSamples.h"
#include "VuoLoopType.h"
#include "VuoVideoFrame.h"
#include "VuoAudioFrame.h"
#include "VuoText.h"
#include "VuoVideoOptimization.h"
#include "VuoUrl.h"

typedef void* VuoVideo;	///< An opaque object for decoding video.

/**
 * Create a new instance of a video player.
 */
VuoVideo VuoVideo_make(VuoUrl path, VuoVideoOptimization optimization);

/**
 * Set the delgate to be called when a new video frame is available.  Video frames will
 * be send to delegates at the appropriate timestamp automatically when playing.
 */
void VuoVideo_setVideoDelegate(VuoVideo player, void(*delegate)(VuoVideoFrame));

/**
 * Set the delgate to be called when a new video frame is available.  Video frames will be
 * sent to delegates at the appropriate timestamp automatically when playing.
 */
void VuoVideo_setAudioDelegate(VuoVideo player, void(*delegate)(VuoList_VuoAudioSamples));

/**
 * Begin playback of video. Frames will be sent to delegates as set by VuoVideo_SetVideoDelegate
 * and VuoVideo_SetAudioDelegate.
 */
void VuoVideo_play(VuoVideo player);

/**
 * Stop playback of video.
 */
void VuoVideo_pause(VuoVideo player);

/**
 * Set the playback rate for the decoder.  If the value is not 1, audio is discarded.
 */
void VuoVideo_setPlaybackRate(VuoVideo player, VuoReal rate);

/**
 * Set how the video reacts when reaching the end of playback.
 */
void VuoVideo_setPlaybackLooping(VuoVideo player, VuoLoopType loop);

/**
 * Return the timestamp of the last decoded video frame.  If no frames have been decoded yet,
 * the timestamp of the first frame is returned.
 */
double VuoVideo_getLastDecodedVideoTimestamp(VuoVideo player);

/**
 * Get the total duration of this movie in seconds.
 */
double VuoVideo_getDuration(VuoVideo player);

/**
 * Seek the decoder to the second.
 */
bool VuoVideo_seekToSecond(VuoVideo player, VuoReal second);

/**
 * Get the video frame at timestamp `second`.  Images contained by VuoVideoFrame have
 * their retain count set at 1 coming from this function.
 */
bool VuoVideo_getFrameAtSecond(VuoVideo player, VuoReal second, VuoVideoFrame* videoFrame);

/**
 * Get the number of audio channels this video contains.
 */
unsigned int VuoVideo_getAudioChannels(VuoVideo player);

/**
 * True if the video is loaded and ready to play, else false.  If false, some values may not yet be available (duration, channel counts, etc)
 */
bool VuoVideo_isReady(VuoVideo player);

/**
 * Steps the video one frame, returning the newly decoded image.  Will be forwards or backwards depending on playback rate.  The image
 * stored in videoFrame will have been registered and have a retain count of 1 - caller is responsible for release.  If returning false,
 * image will be null.
 *
 * If video is currently playing, this will always returns false.
 */
bool VuoVideo_nextVideoFrame(VuoVideo player, VuoVideoFrame* videoFrame);

/**
 * Get the next decoded audio frame.  Returns false if no audio is available, or the playhead is at the end of track.  If true,
 * the audioFrame.samples list will be populated with samples equaliing the number of audio channels.  The `VuoList_VuoAudioSamples()`
 * list should be initialized and retained prior to calling.
 *
 * If video is currently playing, this will always returns false.
 */
bool VuoVideo_nextAudioFrame(VuoVideo player, VuoAudioFrame* audioFrame);

#ifdef __cplusplus
}
#endif
