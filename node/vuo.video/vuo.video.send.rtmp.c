/**
 * @file
 * vuo.video.send.rtmp node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoAudioFrame.h"
#include "VuoImageResize.h"
#include "VuoVideoFrame.h"

#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
	"title": "Send RTMP Video",
	"keywords": [
		"tcp", "ethernet",
		"Real-Time Messaging Protocol",
		"stream", "camera",
		"transmit", "broadcast", "source", "server", "relay",
		"internet",
		"Twitch", "Periscope", "Restream.io", "Wowza", "YouTube Live Streaming", "Facebook Live",
	],
	"version": "1.0.1",
	"dependencies": [
		"VuoFfmpegDecoder",  // For VuoFFmpeg_log.
		"VuoImageResize",
		"VuoUrl",
		"avcodec",
		"avformat",
		"avutil",
		"swresample",
		"swscale",
	],
	"node": {
		"exampleCompositions": [ ],
	}
});

static const VuoReal VuoVideoSendRtmp_WriteTimeThreshold = 0.011;  ///< In seconds.  If sending takes longer than the duration of one 512-byte audio packet at 48kHz (512/48000 ≅ 0.010667), we're going to fall behind.
static const VuoReal VuoVideoSendRtmp_AudioSyncThreshold = 0.2;    ///< How far off, in seconds, audio and video can be before we jump the audio timestamp.
static const VuoReal VuoVideoSendRtmp_StartupStandoff    = 5;      ///< Wait this many seconds before trying to assess (and warn about) dropouts.
static const VuoReal VuoVideoSendRtmp_NoAudioThreshold   = 5;      ///< If the composition hasn't sent audio for this many seconds, injects silent audio.
static const VuoReal VuoVideoSendRtmp_InfoInterval       = 10;     ///< Log status updates after this many seconds.

struct nodeInstanceData
{
	VuoText url;
	VuoText streamKey;
	VuoText username;
	VuoText password;
	int imageWidth;
	int imageHeight;
	VuoReal quality;

	double firstTimestamp;
	double lastInfoTimestamp;
	int64_t cumulativeVideoPacketSize;
	int64_t cumulativeAudioPacketSize;
	int videoFramesSent;
	bool dropout;
	bool warnedAboutDropout;
	int dropoutCount;
	bool warnedAboutFillerAudio;
	bool haveReceivedVideo;
	bool haveReceivedAudio;
	bool haveSentVideo;
	bool haveSentAudio;
	bool connectionEstablished;

	AVFormatContext *formatContext;
	AVStream *videoStream; // @@@ need to release these?
	AVStream *audioStream;
	AVCodecContext *videoCodecContext;
	AVCodecContext *audioCodecContext;
	AVFrame *videoFrame;
	AVFrame *audioFrame;
	bool haveHalfAudioFrame;
	struct SwsContext *scaler;

	VuoImageResize resizer;
};

/**
 * Initializes an RTMP sender with the specified parameters (deallocating the old one if needed).
 */
static void vuo_video_send_rtmp_update(struct nodeInstanceData *context, VuoImage image, VuoText url, VuoText streamKey, VuoText username, VuoText password, VuoReal quality)
{
	// If nothing's changed, leave the existing AVCodecContext intact.
	if (VuoText_areEqual(context->url, url)
	 && VuoText_areEqual(context->streamKey, streamKey)
	 && VuoText_areEqual(context->username, username)
	 && VuoText_areEqual(context->password, password)
	 && (image && context->imageWidth == image->pixelsWide)
	 && (image && context->imageHeight == image->pixelsHigh)
	 && context->quality == quality)
		return;

	if (context->formatContext)
	{
		av_write_trailer(context->formatContext);

		avcodec_free_context(&context->videoCodecContext);
		av_frame_free(&context->videoFrame);

		avcodec_free_context(&context->audioCodecContext);
		av_frame_free(&context->audioFrame);

		sws_freeContext(context->scaler);
		context->scaler = NULL;

		avio_closep(&context->formatContext->pb);
		avformat_free_context(context->formatContext);
		context->formatContext = NULL;
	}

	if (!VuoImage_isPopulated(image) || VuoText_isEmpty(url))
		return;

	// Try to guess reasonable bitrates for quality values between 0 and 1.
	float fudge = 0.75;
	float framerate = 60;
	VuoInteger videoBitrate = MAX(64000, quality * image->pixelsWide * image->pixelsHigh * framerate * fudge);
	VuoInteger audioBitrate = MAX(64000, VuoReal_lerp(64000, 320000, quality));

	char *combinedUrl = NULL;
	bool haveStreamKey = !VuoText_isEmpty(streamKey);
	bool haveAuth = !VuoText_isEmpty(username) && !VuoText_isEmpty(password);
	if (haveStreamKey || haveAuth)
	{
		VuoText scheme, userinfo, host, path, query, fragment;
		VuoInteger port;
		if (VuoUrl_getParts(url, &scheme, &userinfo, &host, &port, &path, &query, &fragment))
		{
			char *portString = port ? VuoText_format(":%llu", port) : NULL;
			char *queryString = query ? VuoText_format("?%s", query) : NULL;
			char *fragmentString = fragment ? VuoText_format("#%s", fragment) : NULL;
			char *userinfoString = haveAuth ? VuoText_format("%s:%s@", username, password) : (userinfo ? VuoText_format("%s@", userinfo) : NULL);
			char *streamKeyString = streamKey ? VuoText_format("/%s", streamKey) : NULL;

			combinedUrl = VuoText_format("%s://%s%s%s%s%s%s%s",
				scheme,
				userinfoString ? userinfoString : "",
				host,
				portString ? portString : "",
				path,
				streamKeyString ? streamKeyString : "",
				queryString ? queryString : "",
				fragmentString ? fragmentString : "");

			VuoRetain(scheme);
			VuoRelease(scheme);
			VuoRetain(userinfo);
			VuoRelease(userinfo);
			VuoRetain(host);
			VuoRelease(host);
			VuoRetain(path);
			VuoRelease(path);
			VuoRetain(query);
			VuoRelease(query);
			VuoRetain(fragment);
			VuoRelease(fragment);
			free(userinfoString);
			free(portString);
			free(streamKeyString);
			free(queryString);
			free(fragmentString);
		}
	}

	if (!combinedUrl)
		combinedUrl = strdup(url);

	VUserLog("Initializing RTMP sender with size %ldx%ld and quality %.2g (%5.2f Mbit/s video, %3.0f Kbit/s audio).",
		image->pixelsWide,
		image->pixelsHigh,
		quality,
		videoBitrate/1000./1000.,
		audioBitrate/1000.);

	context->firstTimestamp = INFINITY;
	context->lastInfoTimestamp      = 0;
	context->cumulativeVideoPacketSize = 0;
	context->cumulativeAudioPacketSize = 0;
	context->videoFramesSent           = 0;
	context->dropout                   = false;
	context->warnedAboutDropout        = false;
	context->dropoutCount              = 0;
	context->warnedAboutFillerAudio    = false;
	context->haveReceivedVideo         = false;
	context->haveReceivedAudio         = false;
	context->haveSentVideo             = false;
	context->haveSentAudio             = false;
	context->connectionEstablished     = false;

	VuoRetain(url);
	VuoRelease(context->url);
	context->url = url;

	VuoRetain(streamKey);
	VuoRelease(context->streamKey);
	context->streamKey = streamKey;

	VuoRetain(username);
	VuoRelease(context->username);
	context->username = username;

	VuoRetain(password);
	VuoRelease(context->password);
	context->password = password;

	context->imageWidth = image->pixelsWide;
	context->imageHeight = image->pixelsHigh;
	context->quality = quality;

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

		avformat_alloc_output_context2(&context->formatContext, NULL, "flv", combinedUrl);
		if (!context->formatContext)
		{
			VUserLog("Error: Couldn't create the format context.");
			return;
		}

		// Since this is a stream, we can't seek back to the beginning to write the duration and filesize in the header.
		av_opt_set(context->formatContext, "flvflags", "no_duration_filesize", AV_OPT_SEARCH_CHILDREN);

		// ======================================================================
		// Video

		// The default, AV_CODEC_ID_FLV1, works with VLC and `ffplay`,
		// but it doesn't work with twitch.tv.
		// AV_CODEC_ID_H264 works with all of the above.
		enum AVCodecID videoCodecID = AV_CODEC_ID_H264;

		AVCodec *videoCodec = avcodec_find_encoder(videoCodecID);
		if (!videoCodec)
		{
			VUserLog("Error: Couldn't find encoder for '%s'.", avcodec_get_name(videoCodecID));
			goto fail;
		}

		context->videoStream = avformat_new_stream(context->formatContext, NULL);
		if (!context->videoStream)
		{
			VUserLog("Error: Couldn't create video stream.");
			goto fail;
		}
		context->videoStream->id = context->formatContext->nb_streams - 1;

		context->videoCodecContext = avcodec_alloc_context3(videoCodec);
		if (!context->videoCodecContext)
		{
			VUserLog("Error: Couldn't create video codec context.");
			goto fail;
		}

		context->videoCodecContext->codec_id = videoCodecID;
		context->videoCodecContext->width  = context->imageWidth;
		context->videoCodecContext->height = context->imageHeight;
		context->videoCodecContext->time_base = context->videoStream->time_base = AV_TIME_BASE_Q;

		// twitch.tv says "[Keyframe] Intervals greater than 4 are unsupported", and recommends 2 seconds.
		// periscope.tv says "Max keyframe interval: 6.0 s" or "3.0 s" in Periscope's low latency mode.
		// loud.wowza.com says "The key frame interval should be equal to or twice the frame rate".
//      context->videoCodecContext->gop_size = 30 /* fps */ * 2 /* seconds */;
		// With FFmpeg 4, GOP > 2 causes `Error: pts < dts` when encoding.
		// https://b33p.net/kosada/vuo/vuo/-/issues/18439
		context->videoCodecContext->gop_size = 2;

		context->videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
		context->videoCodecContext->bit_rate = videoBitrate;
		context->videoCodecContext->bit_rate_tolerance = videoBitrate / 10;
		context->videoCodecContext->qmin = 1;
		context->videoCodecContext->qmax = FF_LAMBDA_MAX;
		context->videoCodecContext->max_qdiff = FF_LAMBDA_MAX;
		context->videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		int ret = avcodec_open2(context->videoCodecContext, videoCodec, NULL);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't open video codec: %s", av_err2str(ret));
			goto fail;
		}

		ret = avcodec_parameters_from_context(context->videoStream->codecpar, context->videoCodecContext);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't copy the video stream parameters: %s", av_err2str(ret));
			goto fail;
		}

		context->videoFrame = av_frame_alloc();
		if (!context->videoFrame)
		{
			VUserLog("Error: Couldn't allocate video frame.");
			goto fail;
		}

		context->videoFrame->format = context->videoCodecContext->pix_fmt;
		context->videoFrame->color_range = AVCOL_RANGE_MPEG;
		context->videoFrame->width  = context->videoCodecContext->width;
		context->videoFrame->height = context->videoCodecContext->height;

		ret = av_frame_get_buffer(context->videoFrame, 32);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't allocate video frame data: %s", av_err2str(ret));
			goto fail;
		}

		// ======================================================================
		// Audio

		enum AVCodecID audioCodecID = AV_CODEC_ID_AAC;

		AVCodec *audioCodec = avcodec_find_encoder(audioCodecID);
		if (!audioCodec)
		{
			VUserLog("Error: Couldn't find encoder for '%s'.", avcodec_get_name(audioCodecID));
			goto fail;
		}

		context->audioStream = avformat_new_stream(context->formatContext, NULL);
		if (!context->audioStream)
		{
			VUserLog("Error: Couldn't create audio stream.");
			goto fail;
		}
		context->audioStream->id = context->formatContext->nb_streams - 1;

		context->audioCodecContext = avcodec_alloc_context3(audioCodec);
		if (!context->audioCodecContext)
		{
			VUserLog("Error: Couldn't create audio codec context.");
			goto fail;
		}

		context->audioCodecContext->codec_id = audioCodecID;
		context->audioCodecContext->sample_fmt = audioCodec->sample_fmts[0];
		context->audioCodecContext->bit_rate = audioBitrate;
		context->audioCodecContext->bit_rate_tolerance = audioBitrate / 10;
		context->audioCodecContext->sample_rate = VuoAudioSamples_sampleRate;
		context->audioCodecContext->channels = 2;
		context->audioCodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
		context->audioCodecContext->time_base = context->audioStream->time_base = AV_TIME_BASE_Q;
		context->audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		ret = avcodec_open2(context->audioCodecContext, audioCodec, NULL);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't open audio codec: %s", av_err2str(ret));
			goto fail;
		}

		if (context->audioCodecContext->frame_size != VuoAudioSamples_bufferSize * 2)
		{
			VUserLog("Error: Expected %lld audio samples per frame; got %d.", VuoAudioSamples_bufferSize * 2, context->audioCodecContext->frame_size);
			goto fail;
		}

		ret = avcodec_parameters_from_context(context->audioStream->codecpar, context->audioCodecContext);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't copy the audio stream parameters: %s", av_err2str(ret));
			goto fail;
		}

		context->audioFrame = av_frame_alloc();
		if (!context->audioFrame)
		{
			VUserLog("Error: Couldn't allocate audio frame.");
			goto fail;
		}

		context->audioFrame->format = context->audioCodecContext->sample_fmt;
		context->audioFrame->channel_layout = context->audioCodecContext->channel_layout;
		context->audioFrame->sample_rate = context->audioCodecContext->sample_rate;
		context->audioFrame->nb_samples = context->audioCodecContext->frame_size;
		context->audioFrame->pts = INT64_MIN;
		context->audioFrame->pkt_duration = VuoAudioSamples_bufferSize * 2. / VuoAudioSamples_sampleRate * AV_TIME_BASE;

		ret = av_frame_get_buffer(context->audioFrame, 0);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't allocate audio frame data: %s", av_err2str(ret));
			goto fail;
		}

		// ======================================================================

		ret = avio_open(&context->formatContext->pb, combinedUrl, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't open url \"%s\": %s", url, av_err2str(ret));
			goto fail;
		}
		VUserLog("Connected to \"%s\".", url);

		ret = avformat_write_header(context->formatContext, NULL);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't write header: %s", av_err2str(ret));
			goto fail;
		}

		context->scaler = sws_getContext(context->imageWidth, context->imageHeight, AV_PIX_FMT_BGR24,
										 context->imageWidth, context->imageHeight, AV_PIX_FMT_YUV420P,
										 0, NULL, NULL, NULL);
		if (!context->scaler)
		{
			VUserLog("Error: Couldn't initialize scaler.");
			goto fail;
		}

		free(combinedUrl);

		context->connectionEstablished = true;

		return;

fail:
		if (context->formatContext)
		{
			if (context->formatContext->pb)
				avio_closep(&context->formatContext->pb);
			avformat_free_context(context->formatContext);
			context->formatContext = NULL;
		}

		if (context->videoCodecContext)
			avcodec_free_context(&context->videoCodecContext);

		if (context->videoFrame)
			av_frame_free(&context->videoFrame);

		if (context->audioCodecContext)
			avcodec_free_context(&context->audioCodecContext);

		if (context->audioFrame)
			av_frame_free(&context->audioFrame);

		free(combinedUrl);
	});
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->resizer = VuoImageResize_make();
	VuoRetain(context->resizer);

	return context;
}

static void vuo_video_send_rtmp_copyAudio(VuoList_VuoAudioSamples channels, AVFrame *audioFrame, int offset)
{
	int16_t *out = (int16_t *)audioFrame->data[0];
	for (int channel = 1; channel <= 2; ++channel)
	{
		VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(channels, channel);
		if (as.samples && as.sampleCount == VuoAudioSamples_bufferSize)
			for (int i = 0; i < VuoAudioSamples_bufferSize; ++i)
				out[offset + i * 2 + channel - 1] = as.samples[i] * 32767;
		else
			for (int i = 0; i < VuoAudioSamples_bufferSize; ++i)
				out[offset + i * 2 + channel - 1] = 0;
	}
}

static bool vuo_video_send_rtmp_sendAudio(struct nodeInstanceData *context)
{
	int ret = avcodec_send_frame(context->audioCodecContext, context->audioFrame);
	if (ret < 0)
	{
		VUserLog("Error: Couldn't encode audio frame: %s", av_err2str(ret));
		return false;
	}

	AVPacket *packet = av_packet_alloc();
	while (true)
	{
		ret = avcodec_receive_packet(context->audioCodecContext, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0)
		{
			VUserLog("Error: Couldn't encode audio frame: %s", av_err2str(ret));
			av_packet_free(&packet);
			return false;
		}

		av_packet_rescale_ts(packet, context->audioCodecContext->time_base, context->audioStream->time_base);
		packet->stream_index = context->audioStream->index;

		context->cumulativeAudioPacketSize += packet->size;

		VuoReal t0 = VuoLogGetElapsedTime();

		ret = av_interleaved_write_frame(context->formatContext, packet);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't send audio frame: %s (%d)", av_err2str(ret), ret);
			av_packet_free(&packet);
			return false;
		}
		VuoReal t1 = VuoLogGetElapsedTime();

		if (t0 - context->firstTimestamp > VuoVideoSendRtmp_StartupStandoff
		 && t1 - t0 > VuoVideoSendRtmp_WriteTimeThreshold)
		{
			context->dropout = true;
			++context->dropoutCount;
			if (!context->warnedAboutDropout)
			{
				VUserLog("Warning: It took %0.2f seconds to send the audio buffer.  This might indicate a network connectivity or bandwidth problem.  I'll skip the next video frame to try to free up bandwidth for audio.  I won't show further warnings about this.", t1 - t0);
				context->warnedAboutDropout = true;
			}
		}
	}
	av_packet_free(&packet);

	context->haveSentAudio = true;
	return true;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoImage) sendImage,
	VuoInputEvent({"eventBlocking":"none", "data":"sendImage"}) sendImageEvent,
	VuoInputData(VuoList_VuoAudioSamples) sendAudio,
	VuoInputEvent({"eventBlocking":"none", "data":"sendAudio"}) sendAudioEvent,
	VuoInputData(VuoText, {"name":"URL"}) url,
	VuoInputData(VuoText) streamKey,
	VuoInputData(VuoText) username,
	VuoInputData(VuoText) password,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":1}) quality)
{
	VuoImage image = sendImage;
	if (sendImageEvent && image && !VuoIsPro() && (image->pixelsWide > 1280 || image->pixelsHigh > 1280))
		image = VuoImageResize_resize(image, (*context)->resizer, VuoSizingMode_Proportional, 1280, 1280);
	VuoLocal(image);

	vuo_video_send_rtmp_update(*context, image, url, streamKey, username, password, quality);

	if (!(*context)->connectionEstablished)
		return;

	if (sendImageEvent)
		(*context)->haveReceivedVideo = true;
	if (sendAudioEvent)
		(*context)->haveReceivedAudio = true;

	if (isinf((*context)->firstTimestamp))
		(*context)->firstTimestamp = VuoLogGetElapsedTime();

	VuoReal currentTimestamp = VuoLogGetElapsedTime() - (*context)->firstTimestamp;

	if (sendImageEvent && image)
	{
		if ((*context)->dropout)
		{
			(*context)->dropout = false;
			return;
		}

		int ret = av_frame_make_writable((*context)->videoFrame);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't create video frame: %s", av_err2str(ret));
			return;
		}

		int width = (*context)->videoCodecContext->width;
		int height = (*context)->videoCodecContext->height;

		const unsigned char *pixels = VuoImage_getBuffer(image, GL_BGR);

		// Flip vertically, without copying, by starting on the last row and using a negative row stride.
		pixels += width * (height - 1) * 3;
		int stride = width * -3;

		// Convert from BGR to YUV.
		sws_scale((*context)->scaler, (const uint8_t * const *)&pixels, &stride, 0, height, (*context)->videoFrame->data, (*context)->videoFrame->linesize);

		(*context)->videoFrame->pts = currentTimestamp * AV_TIME_BASE;

		(*context)->videoFrame->pkt_duration = 1;

		ret = avcodec_send_frame((*context)->videoCodecContext, (*context)->videoFrame);
		if (ret < 0)
		{
			VUserLog("Error: Couldn't encode video frame: %s", av_err2str(ret));
			return;
		}

		AVPacket *packet = av_packet_alloc();
		while (true)
		{
			ret = avcodec_receive_packet((*context)->videoCodecContext, packet);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0)
			{
				VUserLog("Error: Couldn't encode video frame: %s", av_err2str(ret));
				return;
			}

			// `packet`'s timestamps are currently in AV_TIME_BASE; convert them to the stream's timebase.
			av_packet_rescale_ts(packet, (*context)->videoCodecContext->time_base, (*context)->videoStream->time_base);
			packet->stream_index = (*context)->videoStream->index;

			(*context)->cumulativeVideoPacketSize += packet->size;
			if (currentTimestamp - (*context)->lastInfoTimestamp > VuoVideoSendRtmp_InfoInterval)
			{
				VuoReal averageFramerate = (float)(*context)->videoFramesSent / VuoVideoSendRtmp_InfoInterval;
				VuoReal averageTotalBitrate = ((*context)->cumulativeVideoPacketSize + (*context)->cumulativeAudioPacketSize) / (currentTimestamp - (*context)->lastInfoTimestamp) / 1024 / 1024 * 8;
				int64_t currentPTS = currentTimestamp * AV_TIME_BASE;
				VuoReal syncDelta = ((VuoReal)currentPTS - (*context)->audioFrame->pts) / AV_TIME_BASE;
				VUserLog("%2.1f fps, %6.2f Mbit/s video + %6.2f Kbit/s audio = %6.2f Mbit/s total, %d dropout%s, audio %0.3f s %s video",
					averageFramerate,
					(*context)->cumulativeVideoPacketSize / (currentTimestamp - (*context)->lastInfoTimestamp) / 1024 / 1024 * 8,
					(*context)->cumulativeAudioPacketSize / (currentTimestamp - (*context)->lastInfoTimestamp) / 1024 * 8,
					averageTotalBitrate,
					(*context)->dropoutCount,
					(*context)->dropoutCount == 1 ? "" : "s",
					fabs(syncDelta),
					syncDelta > 0 ? "behind" : "ahead of");
				(*context)->lastInfoTimestamp = currentTimestamp;
				(*context)->cumulativeAudioPacketSize = 0;
				(*context)->cumulativeVideoPacketSize = 0;
				(*context)->videoFramesSent = 0;
			}

			VuoReal t0 = VuoLogGetElapsedTime();

			ret = av_interleaved_write_frame((*context)->formatContext, packet);
			if (ret == -EIO || ret == -EPIPE || ret == AVERROR_EOF)
			{
				VUserLog("Error: The server disconnected us (%s).  Attempting to reconnect…", av_err2str(ret));
				vuo_video_send_rtmp_update(*context, NULL, NULL, NULL, NULL, NULL, 0);
				VuoRelease((*context)->url);
				(*context)->url = NULL;
				VuoRelease((*context)->streamKey);
				(*context)->streamKey = NULL;
				VuoRelease((*context)->username);
				(*context)->username = NULL;
				VuoRelease((*context)->password);
				(*context)->password = NULL;
			}
			else if (ret < 0)
				VUserLog("Error: Couldn't send video frame: %s (%d)", av_err2str(ret), ret);

			VuoReal t1 = VuoLogGetElapsedTime();
			if (currentTimestamp > VuoVideoSendRtmp_StartupStandoff
			 && t1 - t0 > VuoVideoSendRtmp_WriteTimeThreshold)
			{
				(*context)->dropout = true;
				++(*context)->dropoutCount;
				if (!(*context)->warnedAboutDropout)
				{
					VUserLog("Warning: It took %0.2f seconds to send the video frame.  This might indicate a network connectivity or bandwidth problem.  I'll skip the next video frame to try to free up bandwidth for audio.  I won't show further warnings about this.", t1 - t0);
					(*context)->warnedAboutDropout = true;
				}
			}

			++(*context)->videoFramesSent;
			(*context)->haveSentVideo = true;
		}
		av_packet_free(&packet);
	}

	if (sendAudioEvent && sendAudio && (*context)->haveSentVideo)
	{
		// An AAC frame (1024 samples) is twice VuoAudioSamples_bufferSize (512 samples).
		if (!(*context)->haveHalfAudioFrame)
		{
			// Copy the first VuoAudioFrame into the first half of the FFmpeg audio frame.

			int ret = av_frame_make_writable((*context)->audioFrame);
			if (ret < 0)
			{
				VUserLog("Error: Couldn't create audio frame: %s", av_err2str(ret));
				return;
			}

			vuo_video_send_rtmp_copyAudio(sendAudio, (*context)->audioFrame, 0);

			(*context)->haveHalfAudioFrame = true;
		}
		else
		{
			// Copy the second VuoAudioFrame into the second half of the FFmpeg audio frame, and send it.

			vuo_video_send_rtmp_copyAudio(sendAudio, (*context)->audioFrame, VuoAudioSamples_bufferSize * (*context)->audioFrame->channels);

			int64_t currentPTS = currentTimestamp * AV_TIME_BASE;
			if ((*context)->audioFrame->pts == INT64_MIN)
				(*context)->audioFrame->pts = currentPTS;

			VuoReal syncDelta = ((VuoReal)currentPTS - (*context)->audioFrame->pts) / AV_TIME_BASE;
			if (syncDelta < -VuoVideoSendRtmp_AudioSyncThreshold
			 || syncDelta > VuoVideoSendRtmp_AudioSyncThreshold)
			{
				(*context)->audioFrame->pts = currentPTS;

				if (currentTimestamp > VuoVideoSendRtmp_StartupStandoff)
				{
					++(*context)->dropoutCount;
					VUserLog("Warning: Audio timestamp is off by %0.2f seconds; jumping in time.", syncDelta);
				}
			}

			vuo_video_send_rtmp_sendAudio(*context);

			(*context)->audioFrame->pts += (*context)->audioFrame->pkt_duration;

			(*context)->haveHalfAudioFrame = false;
		}
	}

	if ((*context)->connectionEstablished
		&& (*context)->haveSentVideo
		&& !(*context)->haveReceivedAudio
		&& (*context)->videoFrame->pts > VuoVideoSendRtmp_NoAudioThreshold * AV_TIME_BASE)
	{
		// If the composition hasn't fed us any audio a few seconds after its first video frame, start sending silent audio,
		// since some RTMP relays (e.g., YouTube and Facebook) only work if we provide both audio and video.

		if (!(*context)->haveSentAudio)
		{
			(*context)->audioFrame->pts = (*context)->videoFrame->pts;

			if (!(*context)->warnedAboutFillerAudio)
			{
				VUserLog("Warning: Since this composition hasn't sent any audio recently, I'm sending silent filler audio, since some RTMP relays require audio.  I won't show further warnings about this.");
				(*context)->warnedAboutFillerAudio = true;
			}
		}

		while ((*context)->audioFrame->pts <= (*context)->videoFrame->pts)
		{
			int ret = av_frame_make_writable((*context)->audioFrame);
			if (ret < 0)
			{
				VUserLog("Error: Couldn't create audio frame: %s", av_err2str(ret));
				return;
			}

			bzero((*context)->audioFrame->data[0], sizeof(int16_t) * (*context)->audioFrame->nb_samples * (*context)->audioFrame->channels);

			if (!vuo_video_send_rtmp_sendAudio(*context))
				break;

			(*context)->audioFrame->pts += (*context)->audioFrame->pkt_duration;
		}
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	vuo_video_send_rtmp_update(*context, NULL, NULL, NULL, NULL, NULL, 0);

	VuoRelease((*context)->url);
	VuoRelease((*context)->streamKey);
	VuoRelease((*context)->username);
	VuoRelease((*context)->password);
	VuoRelease((*context)->resizer);
}
