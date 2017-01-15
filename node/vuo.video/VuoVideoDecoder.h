/**
 * @file
 * VuoVideoDecoder class definition.
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

#include "module.h"
#include "VuoAudioFrame.h"
#include "VuoVideoFrame.h"
#include "VuoUrl.h"

/**
 * Virtual class from which Ffmpeg and AvFoundation video decoders inherit.
 */
class VuoVideoDecoder
{
protected:
	/// Base constructor does nothing interesting.
	VuoVideoDecoder()
	{
		videoPlayer = NULL;
		onReadyToPlay = NULL;
	}

public:

	/// Clean up any resources claimed by decoder.
	virtual ~VuoVideoDecoder() {}

	/// Forward declare VuoVideoPlayer for the onReady delegate.
	typedef class VuoVideoPlayer VideoPlayer;
	/// Reference to the VuoVideoPlayer making use of this decoder.
	VideoPlayer* videoPlayer = NULL;
	/// A delegate to be called when the decoder is ready to begin playback.
	void (VideoPlayer::*onReadyToPlay)(bool canPlayMedia);

	/// Bool relaying the preparedness of playback.  If this is false, register with onReadyToPlay to receive a notification when
	/// playback is enabled.  If inheriting class does not override, playback is assumed to be available immediately following
	/// initialization.
	virtual bool IsReady() { return true; }
	/// Get the next video frame in the queue.  Image comes with a retain count of 1 (as opposed to the usual non-registered value).  If playback speed is negative, this will the the frame prior to the last retrieved.
	virtual bool NextVideoFrame(VuoVideoFrame* frame) = 0;
	/// Get the next audio frame in the queue.  If playback speed is negative (or anything other than 1), this returns nothing.
	virtual bool NextAudioFrame(VuoAudioFrame* audio) = 0;
	/// Seek the playhead to the second.  `second` is not in timestamp format, rather, relative to movie start = 0.
	virtual bool SeekToSecond(double second) = 0;
	/// The total duration of this video in seconds
	virtual double GetDuration() = 0;
	/// Set the playback rate.  Any value that isn't 1 will flush the audio queue and discard any future audio frames until
	/// frame rate is back to 1.
	virtual void SetPlaybackRate(double rate) = 0;
	/// Returns the timestamp of the last decoded video frame.
	virtual double GetLastDecodedVideoTimeStamp() = 0;
	/// Returns true if there isn't an audio track, or there is but the decoder can play it.
	virtual bool CanPlayAudio() = 0;
	/// Returns the number of audio channels in this video;
	virtual unsigned int GetAudioChannelCount() = 0;
	/// Returns the average desired frame-rate.
	virtual double GetFrameRate() = 0;
};


#ifdef __cplusplus
}
#endif
