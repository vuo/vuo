/**
 * @file
 * VuoVideoPlayer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoVideoPlayer.h"
#include "VuoVideoDecoder.h"
#include "VuoAvDecoder.h"
#include "VuoFfmpegDecoder.h"
#include "VuoEventLoop.h"
#include <unistd.h>

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoVideoPlayer",
					 "dependencies" : [
						"VuoVideoDecoder",
						"VuoFfmpegDecoder",
						"VuoAvDecoder",
						"VuoImage",
						"VuoAudioSamples",
						"VuoReal",
						"VuoList_VuoAudioSamples",
						"VuoList_VuoReal"
					 ]
				 });
#endif
}

/// Release and set a video frame to null if it's not null already.
#define TRY_RELEASE_VIDEO(v) { if(v.image != NULL) { VuoRelease(v.image); v.image = NULL; v.timestamp = -1.0; } }

/// Release and set an audio frame to null if it's not null already.
#define TRY_RELEASE_AUDIO(a) { if(a.samples != NULL) { VuoRelease(a.samples); a.samples = NULL; a.timestamp = -1.0; } }

VuoVideoPlayer* VuoVideoPlayer::Create(VuoUrl url, VuoVideoOptimization optimization)
{
	VuoVideoPlayer* player = new VuoVideoPlayer(url);

	player->video_semaphore = dispatch_semaphore_create(0);
	player->audio_semaphore = dispatch_semaphore_create(0);

	player->playbackRate = 1.;
	player->audioEnabled = true;
	player->isPlaying = false;
	player->lastVideoTimestamp = 0.;
	player->lastVideoFrameDelta = 0.;

	/// if optimization is set to forward or auto, assume forward playback.
	player->preferFfmpeg = optimization == VuoVideoOptimization_Random;

	player->decoder = player->preferFfmpeg ? (VuoVideoDecoder*) VuoFfmpegDecoder::Create(player->videoPath) : (VuoVideoDecoder*) VuoAvDecoder::Create(player->videoPath);

	if(player->decoder == NULL || !player->decoder->CanPlayAudio())
	{
		VuoVideoDecoder* alt = player->preferFfmpeg ? (VuoVideoDecoder*) VuoAvDecoder::Create(player->videoPath) : (VuoVideoDecoder*) VuoFfmpegDecoder::Create(player->videoPath);

		if(alt != NULL && (player->decoder == NULL || alt->CanPlayAudio()) )
		{
			if(player->decoder != NULL)
				delete player->decoder;

			player->decoder = alt;

			if(player->preferFfmpeg)
				VDebugLog("Using AvFoundation video decoder despite optimization preference.");
			else
				VDebugLog("Using Ffmpeg video decoder despite optimization preference.");
		}
		else
		{
			VUserLog("AVFoundation and FFmpeg video decoders failed to open the movie.");
			player->failedLoading = true;
		}
	}
	else
	{
		if(player->preferFfmpeg)
			VDebugLog("Using first choice video decoder FFmpeg");
		else
			VDebugLog("Using first choice video decoder AVFoundation");
	}

	player->isReady = player->decoder != NULL && player->decoder->IsReady();

	player->audioFrame.samples = NULL;
	player->videoFrame.image = NULL;

	if(player->decoder == NULL)
	{
		delete player;
		return NULL;
	}
	else
	{
		player->decoder->videoPlayer = player;
		player->decoder->onReadyToPlay = &VuoVideoPlayer::OnDecoderPlaybackReady;
	}

	return player;
}

void VuoVideoPlayer::Destroy()
{
	/// wait til ondecoderplaybackready is invoked before destroying things.
	while( !IsReady() && !failedLoading ) {
		usleep(USEC_PER_SEC / 24);
	}

	delete this;
}

VuoVideoPlayer::~VuoVideoPlayer()
{
	pthread_mutex_lock(&decoderMutex);

	if(isPlaying)
		_Pause();

	dispatch_release(video_semaphore);
	dispatch_release(audio_semaphore);

	if(decoder != NULL)
		delete decoder;

	if(videoPath != NULL)
		VuoRelease(videoPath);

	TRY_RELEASE_VIDEO(videoFrame)
	TRY_RELEASE_AUDIO(audioFrame)

	/// always locked by the Destroy() function prior to deletion
	pthread_mutex_unlock(&decoderMutex);

	int ret =  pthread_mutex_destroy(&decoderMutex);

	switch(ret)
	{
		case EBUSY:
			VUserLog("Failed destroying decoder mutex: EBUSY");
			break;

		case EINVAL:
			VUserLog("Failed destroying decoder mutex: EINVAL");
			break;
	}

}

bool VuoVideoPlayer::IsReady()
{
	return isReady;
}

void VuoVideoPlayer::OnDecoderPlaybackReady(bool canPlayMedia)
{
	pthread_mutex_lock(&decoderMutex);

	if(canPlayMedia)
	{
		if(!decoder->CanPlayAudio())
		{
			VuoVideoDecoder* alt = preferFfmpeg ? (VuoVideoDecoder*) VuoAvDecoder::Create(videoPath) : (VuoVideoDecoder*) VuoFfmpegDecoder::Create(videoPath);

			if(alt != NULL && alt->CanPlayAudio())
			{
				if(decoder != NULL)
					delete decoder;

				decoder = alt;

				if(!alt->IsReady())
					return;
			}
		}

		_SetPlaybackRate(onReadyPlaybackRate);

		if(onReadySeek > -1)
			_Seek(onReadySeek);

		if(playOnReady)
			_Play( dispatch_time(DISPATCH_TIME_NOW, 0) );

		isReady = true;
	}
	else
	{
		if(!preferFfmpeg)
		{
			if(decoder != NULL)
			{
				delete decoder;
				decoder = NULL;
			}

			VuoVideoDecoder* dec = (VuoVideoDecoder*) VuoFfmpegDecoder::Create(videoPath);

			if(dec != NULL)
			{
				decoder = dec;

				_SetPlaybackRate(onReadyPlaybackRate);

				if(onReadySeek > -1)
					_Seek(onReadySeek);

				if(playOnReady)
					_Play(dispatch_time(DISPATCH_TIME_NOW, 0));

				isReady = true;

				VUserLog("On second thought, AVFoundation decided it couldn't play this video.  Fell back on FFMPEG successfully.");
			}
			else
			{
				VUserLog("Both AvFoundation and Ffmpeg failed to load this video.  Try using a different video or audio codec.");
				failedLoading = true;
			}
		}
		else
		{
			VUserLog("Both AvFoundation and Ffmpeg failed to load this video.  Try using a different video or audio codec.");
			failedLoading = true;
		}
	}

	pthread_mutex_unlock(&decoderMutex);
}

void VuoVideoPlayer::Play()
{
	pthread_mutex_lock(&decoderMutex);

	if(!isPlaying)
	{
		if(!IsReady())
		{
			playOnReady = true;

			pthread_mutex_unlock(&decoderMutex);

			return;
		}

		_Play( dispatch_time(DISPATCH_TIME_NOW, 0) );
	}

	pthread_mutex_unlock(&decoderMutex);
}

void VuoVideoPlayer::Pause()
{
	pthread_mutex_lock(&decoderMutex);

	if(isPlaying)
		_Pause();

	pthread_mutex_unlock(&decoderMutex);
}

void VuoVideoPlayer::SetPlaybackRate(double rate)
{
	pthread_mutex_lock(&decoderMutex);

	if(!IsReady())
		onReadyPlaybackRate = rate;
	else
		_SetPlaybackRate(rate);

	pthread_mutex_unlock(&decoderMutex);
}

bool VuoVideoPlayer::Seek(double second)
{
	pthread_mutex_lock(&decoderMutex);

	bool success = false;

	if( !IsReady() )
		onReadySeek = second;
	else
		success = _Seek(second);

	pthread_mutex_unlock(&decoderMutex);

	return success;
}

void VuoVideoPlayer::_Play(dispatch_time_t start)
{
	isPlaying = true;
	playbackStart = start; // dispatch_time(DISPATCH_TIME_NOW, 0);
	timestampStart = decoder->GetLastDecodedVideoTimeStamp();
	lastVideoTimestamp = timestampStart;

	startTimer(&video_timer, start, &video_semaphore, &VuoVideoPlayer::sendVideoFrame);
	startTimer(&audio_timer, start, &audio_semaphore, &VuoVideoPlayer::sendAudioFrame);
}

void VuoVideoPlayer::_Pause()
{
	isPlaying = false;

	if(video_timer != NULL)
		stopTimer(&video_timer, &video_semaphore);

	if(audio_timer != NULL)
		stopTimer(&audio_timer, &audio_semaphore);
}

void VuoVideoPlayer::_SetPlaybackRate(double rate)
{
	bool wasPlaying = isPlaying;

	if(isPlaying)
		_Pause();

	playbackRate = rate < 0 ? fmin(rate, -.00001) : fmax(rate, .00001);
	audioEnabled = VuoReal_areEqual(1., rate);

	decoder->SetPlaybackRate(rate);

	if(wasPlaying)
	{
		if(lastSentVideoTime != DISPATCH_TIME_FOREVER)
		{
			int64_t frame_delta = ((1./decoder->GetFrameRate()) / playbackRate) * NSEC_PER_SEC;
			dispatch_time_t elapsed = MIN( dispatch_time(DISPATCH_TIME_NOW, 0) - lastSentVideoTime, frame_delta );
			dispatch_time_t timer = dispatch_time(DISPATCH_TIME_NOW, frame_delta);
			_Play(timer - elapsed);
		}
		else
		{
			_Play(dispatch_time(DISPATCH_TIME_NOW, 0));
		}
	}
}

bool VuoVideoPlayer::_Seek(double second)
{
	bool wasPlaying = isPlaying;

	if (wasPlaying)
	{
		_Pause();
		if (decoder->SeekToSecond(second, NULL))
		{
			lastSentVideoTime = DISPATCH_TIME_FOREVER;
			_Play( dispatch_time(DISPATCH_TIME_NOW, 0) );
			return true;
		}
	}
	else
	{
		VuoVideoFrame f;
		if (decoder->SeekToSecond(second, &f))
		{
			TRY_RELEASE_VIDEO(videoFrame);
			TRY_RELEASE_AUDIO(audioFrame);
			videoFrame = f;
			return true;
		}
	}

	return false;
}

double VuoVideoPlayer::GetLastFrameDelta()
{
	return lastVideoFrameDelta;
}

double VuoVideoPlayer::GetCurrentTimestamp()
{
	return lastVideoTimestamp;
}

double VuoVideoPlayer::GetDuration()
{
	double dur = 0;

	/// if ffmpeg didn't load, let avfoundation think about it before returning
	while(!IsReady() && !failedLoading)
	{
		usleep(USEC_PER_SEC / 24);
	}

	pthread_mutex_lock(&decoderMutex);
	dur = failedLoading ? 0 : decoder->GetDuration();
	pthread_mutex_unlock(&decoderMutex);

	return dur;
}

unsigned int VuoVideoPlayer::GetAudioChannels()
{
	unsigned int channelCount = 0;

	while(!IsReady() && !failedLoading)
	{
		usleep(USEC_PER_SEC / 24);
	}

	pthread_mutex_lock(&decoderMutex);
	channelCount = failedLoading ? 0 : decoder->GetAudioChannelCount();
	pthread_mutex_unlock(&decoderMutex);

	return channelCount;
}

bool VuoVideoPlayer::GetVideoFrameAtSecond(double second, VuoVideoFrame* frame)
{
	const int MAX_FRAMES_TO_STEP_BEFORE_SEEK = 5;
	const double floatingPointError = 0.0001;
	double averageFrameDuration = 1. / decoder->GetFrameRate();
	double duration = GetDuration();
	double target = fmax(0, fmin(duration, second));

	// first check if the last decoded frame is the one we want, or close to it
	if(videoFrame.image != NULL)
	{
		double frameDelta = fabs(target - videoFrame.timestamp);

		// asking for the frame that's already stored. send it.
		if ( ((videoFrame.timestamp < target + floatingPointError) && (target + floatingPointError < videoFrame.timestamp + videoFrame.duration))
			|| (VuoReal_areEqual(videoFrame.timestamp + videoFrame.duration, duration) && VuoReal_areEqual(target, duration)) )
		{
			// VLog("next frame procured by already matching current (sent  %.3f, %.3f)", videoFrame.timestamp, target);
			*frame = videoFrame;
			return true;
		}
		// asking for the next frame, or very close to it.
		// this depends on the caller having set an appropriate playback rate (either forwards or backwards)
		else if( frameDelta < averageFrameDuration * MAX_FRAMES_TO_STEP_BEFORE_SEEK && (target - videoFrame.timestamp > 0 == playbackRate > 0) )
		{
			for(int i = 0; i < MAX_FRAMES_TO_STEP_BEFORE_SEEK; i++)
			{
				VuoVideoFrame f;
				bool gotNextFrame = NextVideoFrame(&f);

				if(gotNextFrame)
				{
					TRY_RELEASE_VIDEO(videoFrame);
					videoFrame = f;

					if (videoFrame.timestamp + videoFrame.duration > target + floatingPointError)
					{
						*frame = videoFrame;
						// VLog("next frame procured by stepping (sent  %.3f, %.3f)", videoFrame.timestamp, target);
						return true;
					}
				}
			}
		}
	}

	bool seekSuccess = false;

	// if the seek target is ≥ duration seek to a little before the end timestamp and step.
	// that way you usually end up with a valid frame to send.
	if(target > duration - .5)
	{
		seekSuccess = Seek(duration - .5);

		while (videoFrame.timestamp + videoFrame.duration < target + floatingPointError)
		{
			VuoVideoFrame f;

			if( NextVideoFrame(&f) )
			{
				TRY_RELEASE_VIDEO(videoFrame);
				videoFrame = f;
			}
			else
			{
				break;
			}
		}
		// VLog("next frame procured by seek to end & backpedal (sent  %.3f, %.3f)", videoFrame.timestamp, target);
	}
	else
	{
		seekSuccess = Seek(target);
		// VLog("next frame procured by plain ol' seek (sent  %.3f, %.3f)", videoFrame.timestamp, target);
	}

	*frame = videoFrame;

	return seekSuccess;
}

bool VuoVideoPlayer::GetCurrentVideoFrame(VuoVideoFrame* frame)
{
	if(videoFrame.image != NULL)
	{
		*frame = videoFrame;
		return true;
	}
	else
	{
		return false;
	}
}

bool VuoVideoPlayer::NextVideoFrame(VuoVideoFrame* frame)
{
	if(isPlaying || !IsReady())
		return false;

	return decoder->NextVideoFrame(frame);
}

bool VuoVideoPlayer::NextAudioFrame(VuoAudioFrame* frame)
{
	return !isPlaying && IsReady() && decoder->NextAudioFrame(frame);
}

void VuoVideoPlayer::startTimer(dispatch_source_t* timer, dispatch_time_t start, dispatch_semaphore_t* semaphore, void (VuoVideoPlayer::*func)(void))
{
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

	*timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, VuoEventLoop_getDispatchStrictMask(), queue);

	dispatch_source_set_timer(
		*timer,
		start,
		DISPATCH_TIME_FOREVER,
		0);

	dispatch_source_set_event_handler(*timer, ^{  ((*this).*func)(); });
	dispatch_source_set_cancel_handler(*timer, ^{ dispatch_semaphore_signal(*semaphore); });

	dispatch_resume(*timer);
}

void VuoVideoPlayer::stopTimer(dispatch_source_t* timer, dispatch_semaphore_t* semaphore)
{
	dispatch_source_cancel(*timer);
	dispatch_semaphore_wait(*semaphore, DISPATCH_TIME_FOREVER);
	dispatch_release(*timer);

	*timer = NULL;
}

/**
 * If a frame of video is available, send it to the output triggers.  Then attempt to decode the next frame and set
 * the timer to fire according to the frame duration returned from the decoder (and taking into account frame-rate).
 */
void VuoVideoPlayer::sendVideoFrame()
{
	if(videoFrame.image != NULL)
	{
		if(videoFrameReceivedDelegate != NULL)
		{
			lastSentVideoTime = dispatch_time(DISPATCH_TIME_NOW, 0);
			videoFrameReceivedDelegate(videoFrame);
		}

		VuoRelease(videoFrame.image);
		videoFrame.image = NULL;
	}

	double nextFrameEvent = 0;

	if( decoder->NextVideoFrame(&videoFrame) )
	{
		// translate timestamp to one considering playback rate and direction (forawrds or backwards)
		double presentationRelativeTimestamp = ((videoFrame.timestamp - timestampStart) / playbackRate);
		lastVideoFrameDelta = videoFrame.timestamp - lastVideoTimestamp;
		lastVideoTimestamp = videoFrame.timestamp;
		nextFrameEvent = NSEC_PER_SEC * presentationRelativeTimestamp;
	}
	else
	{
		videoFrame.image = NULL;

		// didn't get a frame, so check what the loop type is and do something
		pthread_mutex_lock(&decoderMutex);

		switch(loop)
		{
			case VuoLoopType_None:
				// our work here is done, hang out and wait for further instruction
				isPlaying = false;
				break;

			case VuoLoopType_Loop:
				// seek back to the video start and resume playback
				stopTimer(&audio_timer, &audio_semaphore);
				decoder->SeekToSecond(playbackRate > 0 ? 0 : decoder->GetDuration(), NULL);
				playbackStart = dispatch_time(DISPATCH_TIME_NOW, 0);
				timestampStart = decoder->GetLastDecodedVideoTimeStamp();
				lastVideoTimestamp = timestampStart;
				nextFrameEvent = 0;
				startTimer(&audio_timer, dispatch_time(DISPATCH_TIME_NOW, 0), &audio_semaphore, &VuoVideoPlayer::sendAudioFrame);
				break;

			case VuoLoopType_Mirror:
				stopTimer(&audio_timer, &audio_semaphore);
				playbackRate = -playbackRate;
				audioEnabled = VuoReal_areEqual(1., playbackRate);
				decoder->SetPlaybackRate(playbackRate);
				playbackStart = dispatch_time(DISPATCH_TIME_NOW, 0);
				timestampStart = decoder->GetLastDecodedVideoTimeStamp();
				lastVideoTimestamp = timestampStart;
				nextFrameEvent = 0;
				/// restart the audio timer
				startTimer(&audio_timer, dispatch_time(DISPATCH_TIME_NOW, 0), &audio_semaphore, &VuoVideoPlayer::sendAudioFrame);
				break;
		}

		pthread_mutex_unlock(&decoderMutex);
	}

	if(isPlaying && fabs(playbackRate) > .0001)
	{
		// schedule next frame
		dispatch_source_set_timer(
			video_timer,
			dispatch_time(playbackStart, nextFrameEvent),
			DISPATCH_TIME_FOREVER,
			0);
	}
}

void VuoVideoPlayer::sendAudioFrame()
{
	if( audioFrame.samples != NULL && VuoListGetCount_VuoAudioSamples(audioFrame.samples) > 0 )
	{
		if(audioFrameReceivedDelegate != NULL)
			audioFrameReceivedDelegate(audioFrame.samples);

		VuoRelease(audioFrame.samples);
		audioFrame.samples = NULL;
	}

	audioFrame = VuoAudioFrame_make(VuoListCreate_VuoAudioSamples(), 0);
	VuoRetain(audioFrame.samples);

	if( decoder->NextAudioFrame(&audioFrame) )
	{
		double presentationRelativeTimestamp = ((audioFrame.timestamp - timestampStart) / playbackRate);
		lastAudioTimestamp = audioFrame.timestamp;

		dispatch_source_set_timer(
			audio_timer,
			dispatch_time(playbackStart, NSEC_PER_SEC * presentationRelativeTimestamp),
			DISPATCH_TIME_FOREVER,
			0);
	}
	else
	{
		VuoRelease(audioFrame.samples);
		audioFrame.samples = NULL;
	}
}

