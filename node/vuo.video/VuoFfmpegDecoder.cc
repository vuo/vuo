/**
 * @file
 * VuoFfmpegDecoder implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoFfmpegDecoder.h"
#include <OpenGL/CGLMacro.h>
#include "VuoFfmpegUtility.h"

extern "C"
{

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoFfmpegDecoder",
					 "dependencies" : [
						"VuoImage",
						"VuoAudioFrame",
						"VuoAudioSamples",
						"VuoReal",
						"VuoList_VuoAudioSamples",
						"VuoList_VuoReal",
						"avcodec",
						"avformat",
						"avutil",
						"swscale",
						"swresample"
					 ]
				 });
#endif
}

/// Used in calculating audio offset.
#define AUDIO_DIFF_AVG_NB 20

VuoFfmpegDecoder::VuoFfmpegDecoder(VuoUrl url)
{
	static dispatch_once_t pred;
	dispatch_once(&pred, ^
	{
		avformat_network_init();

		av_log_set_level(VuoIsDebugEnabled() ? AV_LOG_VERBOSE : AV_LOG_FATAL);
	});

	mPlaybackRate = 1.;
	mVideoPath = url;
	VuoRetain(mVideoPath);
}

VuoFfmpegDecoder* VuoFfmpegDecoder::Create(VuoUrl url)
{
	VuoFfmpegDecoder* dec = new VuoFfmpegDecoder(url);

	if(!dec->Initialize())
	{
		delete dec;
		return NULL;
	}
	else
	{
		dec->isReady = true;
		return dec;
	}
}

bool VuoFfmpegDecoder::Initialize()
{
	container.formatCtx = NULL;
	container.videoCodecCtx = NULL;
	container.audioCodecCtx = NULL;
	audio_buf = NULL;
	seeking = false;

	// these will be set with real values in InitializeInfo()
	lastVideoTimestamp = 0;
	lastSentVideoPts = 0;
	lastAudioTimestamp = 0;
	showedTimestampGapWarning = false;
	showedSeekIgnoredWarning = false;

	VDebugLog("Initialize: %s", mVideoPath);

	AVDictionary *opts = 0;
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	int ret = avformat_open_input(&(container.formatCtx), mVideoPath, NULL, &opts);

	// If opening the full normalized URL failed, try again with just the POSIX path.
	// (FFmpeg doesn't seem to like percent-encoded `file:///` URLs.)
	if (ret == AVERROR(ENOENT))
	{
		VuoText path = VuoUrl_getPosixPath(mVideoPath);
		if (path)
		{
			VuoLocal(path);
			ret = avformat_open_input(&(container.formatCtx), path, NULL, &opts);
		}
	}

	if (ret != 0 || !container.formatCtx)
	{
		VUserLog("Error: FFmpeg could not open \"%s\" — %s", mVideoPath, av_err2str(ret));
		return false;
	}

	VDebugLog("FFmpeg context flags: 0x%x", container.formatCtx->ctx_flags);
	if (container.formatCtx->iformat)
		VDebugLog("FFmpeg input format : '%s' (%s)  flags=0x%x  codec=0x%x",
			container.formatCtx->iformat->long_name,
			container.formatCtx->iformat->name,
			container.formatCtx->iformat->flags,
			container.formatCtx->iformat->raw_codec_id);

	// Load video context
	ret = avformat_find_stream_info(container.formatCtx, NULL);
	if (ret < 0)
		VUserLog("Warning: FFmpeg could not find video stream information in \"%s\" — %s.", mVideoPath, av_err2str(ret));

	container.videoStreamIndex = VuoFfmpegUtility::FirstStreamIndexWithMediaType(container.formatCtx, AVMEDIA_TYPE_VIDEO);
	container.audioStreamIndex = VuoFfmpegUtility::FirstStreamIndexWithMediaType(container.formatCtx, AVMEDIA_TYPE_AUDIO);

	if(!InitializeVideo(container))
	{
		VUserLog("Failed initializing video stream.");
		return false;
	}

	// don't care if audio initializes or not, since video can still play without audio
	if( !InitializeAudio(container) )
		audioIsEnabled = false;

	// Set metadata
	if( !InitializeVideoInfo() )
	{
		VUserLog("Error: FFmpeg failed to decode the first video frame.");
		return false;
	}

	// VLog("Duration: %f", GetDuration());
	// VLog("Framerate: %f", av_q2d(container.videoStream->avg_frame_rate));

	return true;
}

bool VuoFfmpegDecoder::InitializeVideo(VuoFfmpegDecoder::AVContainer& container)
{
	if(container.videoStreamIndex < 0)
	{
		VUserLog("Error: FFmpeg could not find a video stream in file \"%s\".", mVideoPath);
		return false;
	}

	container.videoStream = container.formatCtx->streams[container.videoStreamIndex];
	// container.videoCodecCtx->thread_count = 1;

	AVCodec* videoCodec = avcodec_find_decoder(container.videoStream->codecpar->codec_id);

	if(videoCodec == NULL)
	{
		VUserLog("Error: FFmpeg could not find a suitable decoder for file \"%s\".", mVideoPath);
		return false;
	}

	VDebugLog("FFmpeg video codec  : '%s' (%s)",
		videoCodec->long_name,
		videoCodec->name);

	container.videoCodecCtx = avcodec_alloc_context3(videoCodec);
	if (!container.videoCodecCtx)
	{
		VUserLog("Error: FFmpeg could not allocate the decoder context for \"%s\".", mVideoPath);
		return false;
	}
	if (avcodec_parameters_to_context(container.videoCodecCtx, container.videoStream->codecpar) < 0)
	{
		VUserLog("Error: FFmpeg could not find the codec for \"%s\".", mVideoPath);
		return false;
	}


	// Open video packet queue
	videoPackets.destructor = av_packet_unref;
	videoFrames.destructor = VideoFrame::Delete;

	// this will be set in the Initialize() function after audio is also loaded
	lastDecodedVideoPts = 0;
	lastSentVideoPts = 0;
	lastVideoTimestamp = VuoFfmpegUtility::AvTimeToSecond(container.videoStream, lastDecodedVideoPts);

	// Flash can't seek, so when seeking just jet to 0 and step
	if(videoCodec->id == AV_CODEC_ID_FLV1 || videoCodec->id == AV_CODEC_ID_GIF)
		container.seekUnavailable = true;

	// Open codec
	if(avcodec_open2(container.videoCodecCtx, videoCodec, NULL) < 0)
	{
		VUserLog("Error: FFmpeg could not find the codec for \"%s\".", mVideoPath);
		return false;
	}

	return true;
}

bool VuoFfmpegDecoder::InitializeAudio(VuoFfmpegDecoder::AVContainer& container)
{
	if(container.audioStreamIndex < 0)
	{
		audio_channels = 0;
		return false;
	}

	audioPackets.destructor = av_packet_unref;
	audioFrames.destructor = AudioFrame::Delete;

	// And the audio stream (if applicable)
	AVCodec *audioCodec = NULL;

	container.audioStream = container.formatCtx->streams[container.audioStreamIndex];
	// container.audioCodecCtx->thread_count = 1;

	audioCodec = avcodec_find_decoder(container.audioStream->codecpar->codec_id);

	container.audioCodecCtx = avcodec_alloc_context3(audioCodec);
	if (!container.audioCodecCtx)
	{
		VUserLog("Error: FFmpeg could not allocate the decoder context for \"%s\".", mVideoPath);
		return false;
	}
	if (avcodec_parameters_to_context(container.audioCodecCtx, container.audioStream->codecpar) < 0)
	{
		VUserLog("Error: FFmpeg could not find the codec for \"%s\".", mVideoPath);
		return false;
	}

	int ret = -1;
	if (audioCodec == NULL || (ret = avcodec_open2(container.audioCodecCtx, audioCodec, NULL)) < 0)
	{
		VUserLog("Error: Unsupported audio codec %s: %s", avcodec_get_name(container.audioCodecCtx->codec_id), av_err2str(ret));
		// container.audioStreamIndex = -1;
		audio_channels = 0;
		return false;
	}
	else
	{
		VDebugLog("FFmpeg audio codec  : '%s' (%s)",
			audioCodec->long_name,
			audioCodec->name);

		container.swr_ctx = swr_alloc();

		if (!container.swr_ctx)
		{
			VUserLog("Error: FFmpeg could not allocate resampler context.");
			container.audioStreamIndex = -1;
			audio_channels = 0;
			return false;
		}
		else
		{
			/* set output resample options */
			int src_ch_layout = container.audioCodecCtx->channel_layout;
			int src_rate = container.audioCodecCtx->sample_rate;
			audio_channels = container.audioCodecCtx->channels;
			AVSampleFormat src_sample_fmt = container.audioCodecCtx->sample_fmt;

			// we want planar doubles
			AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_DBLP;

			av_opt_set_int(container.swr_ctx, "in_channel_layout", src_ch_layout, 0);
			av_opt_set_int(container.swr_ctx, "in_sample_rate", src_rate, 0);
			av_opt_set_sample_fmt(container.swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

			av_opt_set_int(container.swr_ctx, "out_channel_layout", src_ch_layout, 0);
			av_opt_set_int(container.swr_ctx, "out_sample_rate", VuoAudioSamples_sampleRate, 0);
			av_opt_set_sample_fmt(container.swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

			container.bytesPerAudioSample = av_get_bytes_per_sample(src_sample_fmt);

			 /* averaging filter for audio sync */
			audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
			audio_diff_avg_count = 0;
			audio_diff_cum = .0;

			// Initialize audio_buffer that is used to store data straight from frames, and before
			// they're loaded into VuoAudioSamples arrays.
			audio_buf = (uint8_t **)calloc(container.audioCodecCtx->channels, sizeof(uint8_t *));
			audio_buf_size = 0;
			audio_buf_index = 0;

			// sets default properties and lets ffmpeg manage data
			ret = av_new_packet(&audio_packet, 32);
			if (ret)
			{
				VUserLog("Error: Couldn't create a packet: %s", av_err2str(ret));
				audio_channels = 0;
				container.audioStreamIndex = -1;
				return false;
			}
			audio_pkt_data = NULL;
			audio_pkt_size = 0;
			audioIsEnabled = VuoReal_areEqual(mPlaybackRate, 1.);

			// use 1/30 because humans can't tell the difference less than this threshold
			// 2.0 * 512 / VuoAudioSamples_sampleRate; //container.audioCodecCtx->sample_rate;
			audio_diff_threshold = 1./30;

			if ((ret = swr_init(container.swr_ctx)) < 0)
			{
				VUserLog("Error: Could not initialize audio converter context.  The audio track may be corrupt, or empty.\n%s", av_err2str(ret));
				audio_channels = 0;
				container.audioStreamIndex = -1;
				return false;
			}
		}

		return true;
	}
}

bool VuoFfmpegDecoder::InitializeVideoInfo()
{
	if( !DecodeVideoFrame() )
	{
		VDebugLog("Coudn't find first video frame!");
		return false;
	}

	VideoFrame frame;
	videoFrames.Peek(&frame);

	// first frame is sometimes incorrect - seek to re-orient then test timestamps
	SeekToPts(frame.pts, NULL);

	if(!DecodeVideoFrame())
	{
		VDebugLog("Couldn't decode first video frame (2)");
		return false;
	}

	VideoInfo& vi = container.videoInfo;
	videoFrames.Peek(&frame);
	vi.first_pts = frame.pts;

	if(container.videoStream->duration != AV_NOPTS_VALUE)
		// will be just an estimate until NextVideoFrame gets
		// called to exhaustion
		vi.duration = container.videoStream->duration;
	else
		vi.duration = AV_NOPTS_VALUE;

	vi.last_pts = AV_NOPTS_VALUE;
	vi.max_pts = AV_NOPTS_VALUE;

	SeekToPts(vi.first_pts, NULL);


	if(ContainsAudio())
	{
		if (!DecodeAudioFrame())
			VUserLog("Warning: Couldn't decode the first audio frame.");

		VideoInfo& ai = container.audioInfo;

		AudioFrame aframe;

		if(audioFrames.Peek(&aframe))
		{
			ai.first_pts = aframe.pts;
			ai.last_pts = AV_NOPTS_VALUE;
			ai.duration = container.audioStream->duration;
		}
		else
		{
			ai.first_pts = 0;//AV_NOPTS_VALUE;
			ai.last_pts = AV_NOPTS_VALUE;
			ai.duration = container.audioStream->duration;
		}
	}

	lastVideoTimestamp = VuoFfmpegUtility::AvTimeToSecond(container.videoStream, container.videoInfo.first_pts);
	lastAudioTimestamp = ContainsAudio() ? VuoFfmpegUtility::AvTimeToSecond(container.audioStream, container.audioInfo.first_pts) : -1;

	return true;
}

VuoFfmpegDecoder::~VuoFfmpegDecoder()
{
	if(mVideoPath != NULL)
		VuoRelease(mVideoPath);

	// empty frames
	videoFrames.Clear();
	videoPackets.Clear();

	if(ContainsAudio())
	{
		if(audio_buf != NULL && audio_buf_size > 0)
		{
			for(int i = 0; i < audio_channels; i++)
			{
				free(audio_buf[i]);
			}

			free(audio_buf);
		}

		audioFrames.Clear();
		audioPackets.Clear();
	}

	if(container.formatCtx != NULL) avformat_close_input(&container.formatCtx);
}

/**
 * If there are audio streams, but no channels, that means no codec was found.
 */
bool VuoFfmpegDecoder::CanPlayAudio()
{
	return container.audioStreamIndex < 0 || audio_channels > 0;
}

unsigned int VuoFfmpegDecoder::GetAudioChannelCount()
{
	return audio_channels;
}

/**
 * Decode the next packet on the current context.  May be an audio or video packet, and will be put into it's corresponding packet queue.
 */
bool VuoFfmpegDecoder::NextPacket()
{
	AVPacket packet;

	int ret;
	while((ret = av_read_frame(container.formatCtx, &packet)) >= 0)
	{
		if( packet.stream_index == container.videoStreamIndex ||
			packet.stream_index == container.audioStreamIndex )
		{
			if( packet.stream_index == container.videoStreamIndex )
				videoPackets.Add(packet);
			else if( packet.stream_index == container.audioStreamIndex && audioIsEnabled )
				audioPackets.Add(packet);

			return true;
		}
		else
		{
			av_packet_unref(&packet);
		}
	}

	if (ret != AVERROR_EOF)
		VUserLog("Error: %s", av_err2str(ret));

	return false;
}

bool VuoFfmpegDecoder::NextVideoFrame(VuoVideoFrame* videoFrame)
{
	VideoFrame queuedFrame;

	if( mPlaybackRate >= 0 )
	{
		while(!videoFrames.Shift(&queuedFrame))
		{
			if(!DecodeVideoFrame())
				return false;
		}
	}
	else
	{
		while(!videoFrames.Pop(&queuedFrame))
		{
			if(!DecodePreceedingVideoFrames())
				return false;
		}
	}

	videoFrame->image = queuedFrame.image;
	videoFrame->timestamp = queuedFrame.timestamp;
	videoFrame->duration = queuedFrame.duration;

	lastVideoTimestamp = queuedFrame.timestamp;
	lastSentVideoPts = queuedFrame.pts;

	// if the audio is behind video, put this last frame back in the front of the queue.
	if (audioIsEnabled && !seeking && AudioOffset() < MAX_AUDIO_LATENCY)
	{
		VDebugLog("dup video frame: v: %.3f, a: %.3f => %f", lastVideoTimestamp, lastAudioTimestamp, AudioOffset());
		VuoRetain(queuedFrame.image);
		videoFrames.Unshift(queuedFrame);
	}

	return true;
}

/**
 * This will step frames til the timestamp matches pts - should only be called after seeking since
 * this won't release anything associated iwth frames (decodevideo is smart enough not to read image
 * data if not necessary, and seeking is not necessary)
 */
bool VuoFfmpegDecoder::StepVideoFrame(int64_t pts, VuoVideoFrame *frame)
{
	if (frame)
		frame->image = NULL;

	VideoFrame queuedFrame;

	const double floatingPointError = 0.0001;
	double requestedFrameTime = VuoFfmpegUtility::AvTimeToSecond(container.videoStream, pts);
	do
	{
		while(!videoFrames.Shift(&queuedFrame))
		{
			if(!DecodeVideoFrame())
			{
				return false;
			}
		}
		if (queuedFrame.image)
		{
			if (frame)
			{
				if (frame->image)
					VuoRelease(frame->image);
				*frame = VuoVideoFrame_make(queuedFrame.image, queuedFrame.timestamp, queuedFrame.duration);
			}
			else
				VuoRelease(queuedFrame.image);
		}
	} while (queuedFrame.timestamp + queuedFrame.duration < requestedFrameTime + floatingPointError);

	lastVideoTimestamp = queuedFrame.timestamp;
	lastSentVideoPts = queuedFrame.pts;

	while(videoFrames.Shift(&queuedFrame))
		if (queuedFrame.image)
			VuoRelease(queuedFrame.image);

	return true;
}

bool VuoFfmpegDecoder::StepAudioFrame(int64_t pts)
{
	AudioFrame audioFrame;

	do
	{
		while(!audioFrames.Shift(&audioFrame))
		{
			if(!DecodeAudioFrame())
				return false;
		}

		AudioFrame::Delete(&audioFrame);
	} while(audioFrame.pts < pts);

	lastAudioTimestamp = audioFrame.timestamp;

	// flush whatever's left - shouldn't be much
	while (audioFrames.Shift(&audioFrame))
		AudioFrame::Delete(&audioFrame);

	return true;
}

bool VuoFfmpegDecoder::DecodePreceedingVideoFrames()
{
	// check that we're not already at the beginning
	// populate the video frame queue
	VideoFrame vframe;
	vframe.timestamp = lastVideoTimestamp;

	if(videoFrames.Shift(&vframe))
	{
		if(vframe.pts <= container.videoInfo.first_pts)
		{
			VDebugLog("current frame < 0");
			return false;
		}
	}
	else
	{
		if(lastSentVideoPts <= container.videoInfo.first_pts)
		{
			VDebugLog("current frame < 0");
			return false;
		}
	}

	/// Decode a second's worth of video frames each time stepping back
	const double mReversePlaybackStep = 1.;

	double currentTimestamp = lastVideoTimestamp;
	double seekTarget = currentTimestamp - mReversePlaybackStep;

	// if there are already frames in the queue, this preserves the order
	LLNode<VideoFrame>* first = videoFrames.first;
	LLNode<VideoFrame>* last = videoFrames.last;
	videoFrames.first = NULL;
	videoFrames.last = NULL;

	if( !SeekToSecond(seekTarget, NULL) )
		return false;

	vframe.timestamp = lastVideoTimestamp;

	while(vframe.timestamp < currentTimestamp)
	{
		if(!DecodeVideoFrame())
		{
			break;
		}

		videoFrames.PeekLast(&vframe);
	}

	if(videoFrames.Pop(&vframe))
		VideoFrame::Delete(&vframe);

	// append original frames to frames list
	if(first != NULL)
	{
		videoFrames.last->next = first;
		first->previous = videoFrames.last;
		videoFrames.last = last;
	}

	return true;
}

bool VuoFfmpegDecoder::DecodeVideoFrame()
{
	AVFrame* frame = av_frame_alloc();
	int frameFinished = 0;
	AVPacket packet;
	av_init_packet(&packet);
	unsigned int skips = 0;

SKIP_VIDEO_FRAME:

	while(!frameFinished)
	{
		while(!videoPackets.Shift(&packet))
		{
			if(!NextPacket())
			{
				// https://b33p.net/kosada/node/12217
				// No next packet is available, but FFmpeg may still have a few frames in its internal buffer.
				// So instead of giving up now, call avcodec_decode_video2() with an empty packet to flush it.
				av_new_packet(&packet, 0);
				break;
			}
		}

#if 1
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		avcodec_decode_video2(container.videoCodecCtx, frame, &frameFinished, &packet);
#pragma clang diagnostic pop
#else
		int ret = avcodec_send_packet(container.videoCodecCtx, &packet);
		if (ret == 0
		 || ret == AVERROR(EAGAIN))
		{
			ret = avcodec_receive_frame(container.videoCodecCtx, frame);
			if (ret == 0)
				frameFinished = 1;
			else if (ret == AVERROR(EAGAIN))
				// FFmpeg is telling us we need to send more packets to it before it can decode the frame.
				;
			else
				VUserLog("avcodec_receive_frame error: %s", av_err2str(ret));
		}
		else if (ret == AVERROR(EINVAL))
			// FFmpeg is telling us we need to send more packets to it before it can decode the frame.
			;
		else
			VUserLog("avcodec_send_packet error: %s", av_err2str(ret));
#endif

		if (frameFinished == 0 && packet.size == 0)
		{
			// After we fed it an empty packet, FFmpeg says it doesn't have a frame for us,
			// so decoding is maybe actually finished now.

			VideoInfo& v = container.videoInfo;

			// out of video packets.  last_pts is now accurate.
			if( v.last_pts == AV_NOPTS_VALUE && v.max_pts != AV_NOPTS_VALUE )
				v.last_pts = v.max_pts;

			av_frame_free(&frame);
			return false;
		}

		if (!frameFinished)
			av_packet_unref(&packet);
	}

	if( frameFinished && frame != NULL)
	{
		// Get PTS here because formats with predictive frames can return junk values before a full frame is found
		int64_t pts = frame->best_effort_timestamp;

		// For unknown reasons, FFmpeg sometimes returns large PTS gaps when playing an RTSP stream.
		// https://b33p.net/kosada/node/13972
		int64_t ptsDelta = pts - lastDecodedVideoPts;
		if (packet.duration > 0
		 && lastDecodedVideoPts > 0
		 && ptsDelta > packet.duration * 100
		 && ptsDelta < 0x7000000000000000) // Don't apply this workaround during preroll, which commonly has bogus timestamps.
		{
			if (!showedTimestampGapWarning)
			{
				VUserLog("Warning: The video stream has a large timestamp gap.  Using estimated timestamps instead.");
				showedTimestampGapWarning = true;
			}
			pts = lastDecodedVideoPts + packet.duration;
		}

		int64_t duration = packet.duration == 0 ? pts - lastDecodedVideoPts : packet.duration;
		lastDecodedVideoPts = pts;

		if( container.videoInfo.max_pts == AV_NOPTS_VALUE || lastDecodedVideoPts > container.videoInfo.max_pts )
			container.videoInfo.max_pts = lastDecodedVideoPts;

		if( skips < MAX_FRAME_SKIP && !seeking && AudioOffset() > MAX_AUDIO_LEAD )
		{
			double predicted_timestamp = VuoFfmpegUtility::AvTimeToSecond(container.videoStream, pts + duration);

			// don't skip a frame if we're just going to drop it in the next function - this can
			// happen if the video frame duration is greater than (abs(MAX_AUDIO_LATENCY) + MAX_AUDIO_LEAD).
			if( lastAudioTimestamp - predicted_timestamp > MAX_AUDIO_LATENCY )
			{
				av_packet_unref(&packet);
				av_init_packet(&packet);
				av_frame_free(&frame);
				frame = av_frame_alloc();
				skips++;	// don't skip more than MAX_SFRAME_KIPS frame per-decode
				frameFinished = false;
				goto SKIP_VIDEO_FRAME;
			}
		}

		av_packet_unref(&packet);

		// if seeking and going forward in time, it's okay to skip decoding the image
		VideoFrame vframe = (VideoFrame)
		{
			seeking ? NULL : VuoFfmpegUtility::VuoImageWithAVFrame(container.videoCodecCtx, frame),
			pts,
			VuoFfmpegUtility::AvTimeToSecond(container.videoStream, pts),	// the first video timestamp may not be zero!
			VuoFfmpegUtility::AvTimeToSecond(container.videoStream, duration)
		};

		if(vframe.image != NULL)
			VuoRetain(vframe.image);

		videoFrames.Add(vframe);
		av_frame_free(&frame);

		if(skips > 0)
			VDebugLog("skip frame: v:%f  a:%f  ==> %f", lastVideoTimestamp, lastAudioTimestamp, AudioOffset());

		return true;
	}
	else
	{
		if (frame)
			av_frame_free(&frame);
		av_packet_unref(&packet);
	}

	return false;
}

bool VuoFfmpegDecoder::NextAudioFrame(VuoAudioFrame* audio)
{
	if (!audioIsEnabled || audio->channels == NULL)
		return false;

	// the wanted sample size in bytes to fill each audio channel (in bytes).
	unsigned int sampleSize = VuoAudioSamples_bufferSize * sizeof(double);
	// the current index in the audio samples array that is being appended to from audio_buf (in bytes).
	unsigned int sampleIndex = 0;
	// the size in bytes to copy from each audio_buf channel to audio samples at sampleIndex
	unsigned int copySize = 0;

	// Allocate audio sample vectors.
	for(int i = 0; i < audio_channels; i++)
	{
		VuoAudioSamples samples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
		samples.samplesPerSecond = VuoAudioSamples_sampleRate;
		VuoListAppendValue_VuoAudioSamples(audio->channels, samples);
	}

	// while audio needs more samples to fill
	while(sampleIndex < sampleSize)
	{
		// if the audio_buffer is out of samples, decode more
		if(audio_buf_index >= audio_buf_size)
		{
			if( !FillAudioBuffer() )
				return false;
		}

		audio->timestamp = lastAudioTimestamp;

		// now copy from audio_buf to audio.sampleSize
		copySize = audio_buf_size - audio_buf_index;

		// if audio_buf has too many samples to fit in audio.samples, just copy til audio is full and return
		if(copySize + sampleIndex > sampleSize)
			copySize = sampleSize - sampleIndex;

		for(int i = 0; i < audio_channels; i++)
		{
			VuoAudioSamples samples = VuoListGetValue_VuoAudioSamples(audio->channels, i+1);
			memcpy(samples.samples + sampleIndex/sizeof(double), audio_buf[i] + audio_buf_index, copySize);
		}

		sampleIndex += copySize;
		audio_buf_index += copySize;
	}

	return true;
}

void VuoFfmpegDecoder::ClearAudioBuffer()
{
	// first order of business is to free the old buffer
	if(audio_buf_size > 0)
	{
		for(int i = 0; i < audio_channels; i++)
		{
			if(audio_buf[i] != NULL)
				free( audio_buf[i] );
		}
	}
	if (audio_buf)
	{
		free(audio_buf);
		audio_buf = NULL;
	}

	// now reset the audio_buf_index and audio_buf_size to match the new decoded frame
	audio_buf_index = 0;
	audio_buf_size = 0;
}

bool VuoFfmpegDecoder::FillAudioBuffer()
{
	ClearAudioBuffer();

	AudioFrame audioFrame;

	while(!audioFrames.Shift(&audioFrame))
	{
		if(!DecodeAudioFrame())
		{
			lastAudioTimestamp = -1;//audioFrame.timestamp;
			return false;
		}
	}

	lastAudioTimestamp = audioFrame.timestamp;
	audio_buf = audioFrame.samples;
	audio_buf_size = audioFrame.size;

	return true;
}

double VuoFfmpegDecoder::AudioOffset()
{
	if( ContainsAudio() && audioIsEnabled && lastAudioTimestamp != -1 )
		return lastAudioTimestamp - lastVideoTimestamp;
	else
		return 0.;
}

bool VuoFfmpegDecoder::DecodeAudioFrame()
{
	AVFrame* frame = av_frame_alloc();
	container.audioCodecCtx->request_sample_fmt = AV_SAMPLE_FMT_FLTP;

	// don't bother allocating samples array if seeking since we're just decoding packets for timestamps
	uint8_t** samples = seeking ? NULL : (uint8_t**)malloc(sizeof(uint8_t*) * audio_channels);

	int len1;
	int converted_sample_count = 0;

	for(;;)
	{
		// int packetsSent = 0;

		while(audio_pkt_size > 0)
		{
			int got_frame = 0;

#if 1
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			len1 = avcodec_decode_audio4(container.audioCodecCtx, frame, &got_frame, &audio_packet);
#pragma clang diagnostic pop
#else
			int ret = avcodec_send_packet(container.audioCodecCtx, &audio_packet);
			++packetsSent;
			if (ret == 0
			 || ret == AVERROR(EAGAIN))
			{
				ret = avcodec_receive_frame(container.audioCodecCtx, frame);
				if (ret == 0)
				{
					got_frame = 1;
					len1 = audio_packet.size;
				}
				else if (ret == AVERROR(EAGAIN))
				{
					// FFmpeg is telling us we need to send more packets to it before it can decode the frame.
					// But sometimes it's never satiated, so give up after we've tried feeding it a few times.
					// Fixes hang in `TestVuoVideo::testDecodePerformance(MPEG v4 HE AAC opt=1)`
					if (packetsSent > 5)
						return false;
				}
				else
					VUserLog("avcodec_receive_frame error: %s", av_err2str(ret));
			}
			else if (ret == AVERROR(EINVAL))
				// FFmpeg is telling us we need to send more packets to it before it can decode the frame.
				;
			else
			{
				VUserLog("avcodec_send_packet error: %s", av_err2str(ret));
				// Sometimes, after seeking, just the first few packets are invalid,
				// or all the packets are invalid.  Give up after a few tries.
				if (ret == AVERROR_INVALIDDATA && packetsSent > 5)
					return false;
			}
#endif

			/* if error, skip frame */
			if(len1 < 0)
			{
				audio_pkt_size = 0;
				break;
			}

			audio_pkt_data += len1;
			audio_pkt_size -= len1;

			if (got_frame)
			{
				int64_t pts = frame->best_effort_timestamp;

				if(seeking)
				{
					AudioFrame audioFrame = {
						0,
						audio_channels,
						pts,
						VuoFfmpegUtility::AvTimeToSecond(container.audioStream, pts),
						NULL
					};

					audioFrames.Add(audioFrame);

					av_frame_free(&frame);
					return true;
				}

				lastDecodedAudioPts = pts;

				// convert frame data to double planar
				uint8_t **dst_data;
				int dst_linesize;

				// figure out how many samples should come out of swr_convert
				int dst_nb_samples = av_rescale_rnd(swr_get_delay(container.swr_ctx, container.audioCodecCtx->sample_rate) +
					frame->nb_samples, VuoAudioSamples_sampleRate, container.audioCodecCtx->sample_rate, AV_ROUND_UP);

				/* allocate and fill destination double* arrays */
				int ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, audio_channels, dst_nb_samples, AV_SAMPLE_FMT_DBLP, 0);

				if(ret < 0)
				{
					VUserLog("av_samples_alloc_array_and_samples error: %s", av_err2str(ret));
					free(samples);
					return false;
				}

				/**
				 *	ret returns the new number of samples per channel
				 */
				ret = swr_convert(container.swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
				if(ret < 0) VUserLog("Failed conversion!");

				converted_sample_count = ret;

				/*
				 * For planar sample formats, each audio channel is in a separate data plane, and linesize is the
				 * buffer size, in bytes, for a single plane. All data planes must be the same size. For packed
				 * sample formats, only the first data plane is used, and samples for each channel are interleaved.
				 * In this case, linesize is the buffer size, in bytes, for the 1 plane.
				 */
				for(int i = 0; i < audio_channels; i++)
				{
					samples[i] = (uint8_t*)malloc(dst_linesize);
					memcpy(samples[i], dst_data[i], dst_linesize);
				}

				av_frame_free(&frame);

				if (dst_data)
				{
					av_freep(&dst_data[0]);
					av_freep(&dst_data);
				}

				AudioFrame audioFrame = {
					(unsigned int)(converted_sample_count * sizeof(double)),
					audio_channels,
					pts,
					VuoFfmpegUtility::AvTimeToSecond(container.audioStream, pts),
					samples
				};

				audioFrames.Add(audioFrame);

				return true;
			}
		}

		if(audio_pkt_data != NULL)
			av_packet_unref(&audio_packet);

		while( !audioPackets.Shift(&audio_packet) )
		{
			if(!NextPacket())
			{
				audio_pkt_data = NULL;
				audio_pkt_size = 0;
				return false;
			}
		}

		audio_pkt_data = audio_packet.data;
		audio_pkt_size = audio_packet.size;
	}
}

bool VuoFfmpegDecoder::SeekToSecond(double second, VuoVideoFrame *frame)
{
	// Convert second to stream time
	int64_t pts = VuoFfmpegUtility::SecondToAvTime(container.videoStream, fmax(second, 0));
	SeekToPts(pts, frame);
	return true;
}

/**
 * Seek to timestamp in video stream time-base.
 * @version200Changed{Changed return type to `void`.}
 */
void VuoFfmpegDecoder::SeekToPts(int64_t pts, VuoVideoFrame *frame)
{
	int64_t target_pts = pts;

	// flush queues
	videoPackets.Clear();
	videoFrames.Clear();

	avcodec_flush_buffers(container.videoCodecCtx);

	if(ContainsAudio())
	{
		ClearAudioBuffer();
		audioPackets.Clear();
		audioFrames.Clear();
		avcodec_flush_buffers(container.audioCodecCtx);
	}

	// seek video & audio
	int ret = 0;
	if (container.formatCtx->iformat->flags & AVFMT_NOFILE)
	{
		if (!showedSeekIgnoredWarning)
		{
			VUserLog("Warning: Ignoring seeks, since this is a stream (not a file).");
			showedSeekIgnoredWarning = true;
		}
	}
	else
		ret = av_seek_frame(container.formatCtx, container.videoStreamIndex, target_pts, AVSEEK_FLAG_BACKWARD);

	if(ret < 0)
		VDebugLog("Warning: av_seek_frame() failed: %s", av_err2str(ret));

//	seeking = true;

	// before seeking, set a "best guess" timestamp so that if the seek was to the end of video
	// and no stepping is required, the timestamp is still (somewhat) accurate
	lastVideoTimestamp = VuoFfmpegUtility::AvTimeToSecond(container.videoStream, target_pts);
	lastSentVideoPts = target_pts;

	// step video and audio til the frame timestamp matches pts
	if (!StepVideoFrame(pts, frame))
		VUserLog("Warning: Couldn't seek video.");

	if(ContainsAudio())
	{
		int64_t audioPts = av_rescale_q(pts, container.videoStream->time_base, container.audioStream->time_base);

		if( audioIsEnabled )
			if (!StepAudioFrame(audioPts))
				VUserLog("Warning: Couldn't seek audio.");
	}

//	seeking = false;
}

bool VuoFfmpegDecoder::ContainsAudio()
{
	return audio_channels > 0;
}

double VuoFfmpegDecoder::GetDuration()
{
	int64_t duration = container.videoInfo.duration;

	if(duration == AV_NOPTS_VALUE)
	{
		if(container.videoInfo.last_pts == AV_NOPTS_VALUE)
		{
			if (container.formatCtx->iformat->flags & AVFMT_NOFILE)
			{
				// For streaming video sources (e.g., RTSP), just use the reported duration,
				// rather than seeking to the end (which could take a long time).
				container.videoInfo.duration = container.formatCtx->duration;
				return VuoFfmpegUtility::AvTimeToSecond(container.videoStream, container.formatCtx->duration);
			}
			else
			{
				// need to manually run through video til end to get last pts value
				seeking = true;
				if (!StepVideoFrame(INT64_MAX, NULL))
					VUserLog("Warning: Couldn't seek to end of video.");
				seeking = false;
				container.videoInfo.duration = container.videoInfo.last_pts - container.videoInfo.first_pts;
			}
		}

		return VuoFfmpegUtility::AvTimeToSecond(container.videoStream, container.videoInfo.last_pts) - VuoFfmpegUtility::AvTimeToSecond(container.videoStream, container.videoInfo.first_pts);
	}
	else
	{
		double u = VuoFfmpegUtility::AvTimeToSecond(container.videoStream, container.videoInfo.duration);
		return u;
	}
}

void VuoFfmpegDecoder::SetPlaybackRate(double rate)
{
	bool audioWasEnabled = audioIsEnabled;
	audioIsEnabled = (ContainsAudio() && fabs(rate - 1.) < .00001);

	if( (!audioWasEnabled && audioIsEnabled) || rate > 0 != mPlaybackRate > 0 )
	{
		mPlaybackRate = rate;
		SeekToSecond(lastVideoTimestamp, NULL);
	}
	else
	{
		mPlaybackRate = rate;
	}
}

double VuoFfmpegDecoder::GetLastDecodedVideoTimeStamp()
{
	return lastVideoTimestamp;
}

double VuoFfmpegDecoder::GetFrameRate()
{
	return av_q2d(container.videoStream->avg_frame_rate);
}
