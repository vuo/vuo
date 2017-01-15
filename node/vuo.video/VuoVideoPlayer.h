/**
 * @file
 * VuoVideoPlayer interface.
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
#include "VuoVideoOptimization.h"
#include <pthread.h>

/**
 * Video playback using a decoder to read video.
 */
class VuoVideoPlayer
{
private:

	/// Private constructor, use VuoVideoPlayer::Create to initialize a new instance.
	VuoVideoPlayer(VuoUrl url) :
		playbackRate(1),
		playOnReady(false),
		onReadySeek(-1),
		onReadyPlaybackRate(1),
		lastVideoFrameDelta(0),
		loop(VuoLoopType_None)
	{
		failedLoading = false;
		isReady = false;

		videoPath = VuoUrl_normalize(url, false);
		VuoRetain(videoPath);

	    pthread_mutex_init(&decoderMutex, NULL);

		videoFrameReceivedDelegate = NULL;
		audioFrameReceivedDelegate = NULL;
	}

	/// Private destructor.  Use VuoVideoPlayer::Destroy() instead.
	~VuoVideoPlayer();

	/// How fast and in which direction the video is being decoded. Negative values are allowed.  A value of 1 means
	/// video is playing at the speed requested by the file, where 2 is twice as fast, and -1 is reversed.
	double playbackRate;

	/// Release resources associated with this instance.
	class VuoVideoDecoder* decoder;

	/// the dispatch time that playback began
	dispatch_time_t playbackStart;

	/// the timestamp that playback began on (from video stream, converted to seconds)
	double timestampStart;

	/// Timer for video thread
	dispatch_source_t video_timer;

	/// Semaphore for video thread
	dispatch_semaphore_t video_semaphore;

	/// Timer for audio thread
	dispatch_source_t audio_timer;

	/// Semaphore for audio thread
	dispatch_semaphore_t audio_semaphore;

	/// time that next video frame send event is scheduled
	dispatch_time_t lastSentVideoTime;

	/// The last decoded video frame.
	VuoVideoFrame videoFrame;

	/// The last decoded set of audio samples.
	VuoAudioFrame audioFrame;

	/// Is audio currently enabled (media must contain audio, and playback rate equal to 1)
	bool audioEnabled;

	/// Is the video currently playing
	bool isPlaying;

	/// True when this player is ready to begin playback
	bool isReady;

	/// True if both AVFoundation and FFMPEG fail loading media.
	bool failedLoading;

	/// If play() has been called, but the decoder isn't finished loading, this will be set to true and play will be
	/// called as soon as OnDecoderPlaybackReady is raised.
	bool playOnReady;

	/// If seek is called before the decoder is available, store the desired value til it is ready.
	double onReadySeek;

	/// If SetPlaybackRate is called before the decoder is available, store the desired value til it is ready.
	double onReadyPlaybackRate;

	/// The last video timestamp received from the video decoder
	double lastVideoTimestamp;

	/// The current video timestamp - the previously received video timestamp
	double lastVideoFrameDelta;

	/// The last audio timestamp received from the video decoder
	double lastAudioTimestamp;

	/// Sends a decoded video frame to the `videoFrameReceivedDelegate` function (if non-null).
	void sendVideoFrame();

	/// Sends a decoded audio frame to the `audioFrameReceivedDelegate` function (if non-null).
	void sendAudioFrame();

	/// Starts a timer, handling creating `timer` thread and resuming timing.
	void startTimer(dispatch_source_t* timer, dispatch_time_t start, dispatch_semaphore_t* semaphore, void (VuoVideoPlayer::*func)(void));

	/// Stops `timer`, releasing it in the process.  Does not release `semaphore`.
	void stopTimer(dispatch_source_t* timer, dispatch_semaphore_t* semaphore);

	/// A delegate raised by the decoder when it is ready to begin playback.
	void OnDecoderPlaybackReady(bool canPlayMedia);

	/// Indicate the VuoVideoOptimization preference.
	bool preferFfmpeg;

	/// restrict access to the decoder when changing playback settings
	pthread_mutex_t decoderMutex;

	/// Internal implementation bypasses decoder mutex lock
	void _SetPlaybackRate(double rate);

	/// Internal implementation bypasses decoder mutex lock
	void _Play(dispatch_time_t start);

	/// Internal implementation bypasses decoder mutex lock
	void _Pause();

	/// Internal implementation bypasses decoder mutex lock
	bool _Seek(double second);

public:

	/**
	 * Create a new video player instance.  If url is not found, or the media is playable `Create` will return null and
	 * clean up any resources allocated in the process.
	 */
	static VuoVideoPlayer* Create(VuoUrl url, VuoVideoOptimization optimization);

	/// Release resources used by this player instance.
	void Destroy();

	/// The path to the movie file.
	VuoUrl videoPath;

	/// Delegate to call when a new video frame is available
	void (* videoFrameReceivedDelegate)(VuoVideoFrame);

	/// Delegate to call when a new audio frame is available
	void (* audioFrameReceivedDelegate)(VuoList_VuoAudioSamples);

	/// What to do when playback is finished
	VuoLoopType loop;

	/**
	 * Set the pointer to the static function that will be called when a new video frame is available.
	 * The player handles timing.
	 */
	void SetVideoDelegate(void(*func)(VuoVideoFrame)) {
		videoFrameReceivedDelegate = func;
	}

	/**
	 * Set the pointer to the static function that will be called when a new audio frame is available.
	 */
	void SetAudioDelegate(void(*func)(VuoList_VuoAudioSamples)) {
		audioFrameReceivedDelegate = func;
	}

	/// Set the playback speed.  Can be negative or positive.  Events to videoFrameReceivedDelegate will be sent at a rate of `video fps * rate`.
	void SetPlaybackRate(double rate);
	/// Returns true if media contains audio, false otherwise.
	// bool ContainsAudio();
	/// Begins decoding and sending video/audio frames through delegates.
	void Play();
	/// Stop playback (but don't move the playhead).
	void Pause();
	/// Seek to a position in the video track.  0 is always the start of playback.  `second` will be
	/// clamped to 0 ≤ `second` ≤ duration.
	bool Seek(double second);
	/// Returns the last decoded video frame timestamp.
	double GetCurrentTimestamp();
	/// Returns the duration in seconds of the video associated with this player.
	double GetDuration();
	/// Get the last decoded image.  This may be null, and will return false in the event that no frame
	/// is available.  Images associated with returned frames have a retain count of 1.
	bool GetCurrentVideoFrame(VuoVideoFrame* frame);
	/// Return the number of audio channels available.
	unsigned int GetAudioChannels();
	/// True if the video is loaded and ready to play, else false.  If false, some values may not yet be available (duration, channel counts, etc)
	bool IsReady();
	/// Get the next video frame.  Always false if currently playing.
	bool NextVideoFrame(VuoVideoFrame* frame);
	/// Get the next audio frame.  Always false if currently playing.
	bool NextAudioFrame(VuoAudioFrame* frame);
};

#ifdef __cplusplus
}
#endif
