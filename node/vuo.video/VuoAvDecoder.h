/**
 * @file
 * VuoAvDecoder class definition.
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

#include "VuoVideoDecoder.h"
#include "VuoAvPlayerInterface.h"
#include "module.h"
#include "VuoAudioFrame.h"
#include "VuoVideoFrame.h"
#include "VuoReal.h"
#include "VuoUrl.h"

/**
 * AvFoundation video decoder.
 */
class VuoAvDecoder : public VuoVideoDecoder
{
private:
	/// VuoAvPlayerObject pointer
	VuoAvPlayerObjPtr player;

protected:
	/// Constructor initializes the decoder object.
	VuoAvDecoder() : VuoVideoDecoder() {
		queue = dispatch_queue_create("org.vuo.avfoundation", NULL);
	}
	dispatch_queue_t queue;	///< Serializes all operations, so this class's methods can be called from any thread.

public:

	/// Initialize a new AvFoundation video decoder with URL.
	static VuoAvDecoder* Create(VuoUrl url);

	/// Clean up any resources claimed by decoder.
	virtual ~VuoAvDecoder();
	/// Bool relaying the preparedness of playback.  If this is false, register with onReadyToPlay to receive a notification when
	/// playback is enabled.  If inheriting class does not override, playback is assumed to be available immediately following
	/// initialization.
	virtual bool IsReady();
	/// Get the next video frame in the queue.  If playback speed is negative, this will the the frame prior to the last retrieved.
	virtual bool NextVideoFrame(VuoVideoFrame* frame);
	/// Get the next audio frame in the queue.  If playback speed is negative (or anything other than 1), this returns nothing.
	virtual bool NextAudioFrame(VuoAudioFrame* audio);
	/// Seek the playhead to the second.  `second` is not in timestamp format, rather, relative to movie start.
	virtual bool SeekToSecond(double second);
	/// The total duration of this video in seconds
	virtual double GetDuration();
	/// Set the playback rate.  Any value that isn't 1 will flush the audio queue and discard any future audio frames until
	/// frame rate is back to 1.
	virtual void SetPlaybackRate(double rate);
	/// Returns the timestamp of the last decoded video frame.
	virtual double GetLastDecodedVideoTimeStamp();
	/// Returns true if there isn't an audio track, or there is but the decoder can play it.
	virtual bool CanPlayAudio();
	/// Returns the number of audio channels in this video;
	virtual unsigned int GetAudioChannelCount();
	/// Called by AvFoundation when ready to begin playback. If VuoVideoDecoder videoPlayer and onReadyToPlay are non-null,
	/// they'll be invoked.
	void DecoderReady(bool canPlayMedia);
	/// The average video framerate.
	double GetFrameRate();

};


#ifdef __cplusplus
}
#endif
