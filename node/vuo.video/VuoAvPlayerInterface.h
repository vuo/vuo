/**
 * @file
 * Vuo AV Foundation Player interface implementation.
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

#include "node.h"
#include "VuoAudioFrame.h"
#include "VuoVideoFrame.h"

/**
 * Pointer to a VuoAvPlayerObject, used to play videos using AV Foundation.
 */
typedef void* VuoAvPlayerObjPtr;

/**
 *	Create a new VuoAvPlayer instance.
 */
VuoAvPlayerObjPtr VuoAvPlayer_make(const char* url);

/**
 * Release a VuoAvPlayer instance.
 */
void VuoAvPlayer_release(VuoAvPlayerObjPtr player);

/**
 * Is the decoder ready to begin playback?
 */
bool VuoAvPlayer_isReady(VuoAvPlayerObjPtr player);

/**
 * Set the rate at which video plays (forwards or backwards).
 */
void VuoAvPlayer_setPlaybackRate(VuoAvPlayerObjPtr player, VuoReal rate);

/**
 * The average framerate.
 */
double VuoAvPlayer_getFrameRate(VuoAvPlayerObjPtr player);

/**
 * Seek playhead to second.
 */
bool VuoAvPlayer_seekToSecond(VuoAvPlayerObjPtr player, VuoReal second);

/**
 * Set the VuoVideoPlayer instance to call OnDecoderPlaybackReady
 */
void VuoAvPlayer_setOnPlaybackReadyCallbackObject(VuoAvPlayerObjPtr player, void (*callback)(void* id, bool canPlayMedia), void* id);

/**
 * Get the next video frame (forwards or backwards).
 */
bool VuoAvPlayer_nextVideoFrame(VuoAvPlayerObjPtr player, VuoVideoFrame* videoFrame);

/**
 * Get the next audio frame.
 */
bool VuoAvPlayer_nextAudioFrame(VuoAvPlayerObjPtr player, VuoAudioFrame* audioFrame);

/**
 * Return the last decoded and accessed timestamp.
 */
VuoReal VuoAvPlayer_getCurrentTimestamp(VuoAvPlayerObjPtr player);

/**
 * Get the duration of this video.
 */
VuoReal VuoAvPlayer_getDuration(VuoAvPlayerObjPtr player);

/**
 * Return true if the asset either has no audio, or it does and can play it.
 */
bool VuoAvPlayer_canPlayAudio(VuoAvPlayerObjPtr player);

/**
 * Return the number of audio channels available.
 */
unsigned int VuoAvPlayer_audioChannelCount(VuoAvPlayerObjPtr player);

#ifdef __cplusplus
}
#endif
