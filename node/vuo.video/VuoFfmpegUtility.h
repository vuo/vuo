/**
 * @file
 * VuoFfmpegUtility interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#include "module.h"
#include "VuoImage.h"
#include "VuoAudioSamples.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <string.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#pragma clang diagnostic pop
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

/**
 * Helper methods for working with ffmpeg.
 */
namespace VuoFfmpegUtility
{
	/// Multiply by this to convert microsecond to seconds.
	const double USEC_TO_SECOND = .000001;

	/// Multiply a second by this value to convert to microsecond.
	const double SEC_TO_USEC = 1000000;

	/// Converts AVStream presentation timestamp to fractional seconds.
	static double AvTimeToSecond(AVStream* stream, int64_t pts)
	{
		if (pts == INT64_MAX)
			return DBL_MAX;
		return av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q) * USEC_TO_SECOND;
	}

	/// Converts AVStream presentation timestamp to integer microseconds.
	static int64_t AvTimeToMicrosecond(AVStream* stream, int64_t pts)
	{
		if( pts == AV_NOPTS_VALUE )
			VUserLog("Timestamp value is invalid!");
		return av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
	}

	/// Converts seconds to microseconds
	static int64_t SecondToAvTime(AVStream* stream, double second)
	{
		// convert second to millisecond
		int64_t ms = av_rescale(second * 1000, stream->time_base.den, stream->time_base.num);

		// convert to microsecond
		return ms / 1000;
	}

	/**
	 * Return the index of the first stream matching the media type.
	 */
	static int FirstStreamIndexWithMediaType(AVFormatContext* context, AVMediaType type)
	{
		for(int i = 0; i < context->nb_streams; i++)
			if( context->streams[i]->codecpar->codec_type == type)
				return i;
		return -1;
	}

	/**
	 * Flip an image vertically.
	 */
	static void FlipImageBytesVertical(uint8_t* buffer, uint width, uint height)
	{
		uint row_size_bytes = width * sizeof(uint8_t);
		uint8_t* tmp = (uint8_t*)malloc(row_size_bytes);

		for(uint i = 0; i < height/2; i++)
		{
			memcpy(tmp, buffer + width * i, row_size_bytes);
			memcpy(buffer + width * i, buffer + width * (height-i-1), row_size_bytes);
			memcpy(buffer + width * (height-i-1), tmp, row_size_bytes);
		}
		free(tmp);
	}

	/// Whether we've already shown a warning about using the slow flipper code path.
	bool warnedAboutSlowFlip = false;

	/**
	 * Convert an AVFrame from whatever format it was originally in to RGB.
	 */
	static bool ConvertAVFrameToRGB(AVFrame *dst, AVPixelFormat dst_pix_fmt, AVFrame *src, AVPixelFormat pix_fmt, int width, int height)
	{
		SwsContext *img_convert_ctx = sws_getContext(width, height, pix_fmt, width, height, dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

		bool slowFlip = false;
		int planeCount = av_pix_fmt_count_planes((AVPixelFormat)src->format);

		// Flip vertically without copying by starting on the last row and using a negative row stride.
		if (planeCount == 1
		 || src->format == AV_PIX_FMT_YUV422P
		 || src->format == AV_PIX_FMT_YUVJ422P
		 || src->format == AV_PIX_FMT_YUV422P10LE
		 || src->format == AV_PIX_FMT_YUVJ444P
		 || src->format == AV_PIX_FMT_YUVA444P12LE
		 || src->format == AV_PIX_FMT_YUV411P
		 || src->format == AV_PIX_FMT_YUVJ411P)
		{
			// All planes have the same height.
			for (int i = 0; i < planeCount; ++i)
			{
				src->data[i] += src->linesize[i] * (src->height - 1);
				src->linesize[i] = -src->linesize[i];
			}
		}
		else if (src->format == AV_PIX_FMT_YUV420P
			  || src->format == AV_PIX_FMT_YUVJ420P)
		{
			// 3 planes, last 2 half height.
			for (int i = 0; i < 3; ++i)
			{
				if (i == 0)
					src->data[i] += src->linesize[i] * (src->height - 1);
				else
					src->data[i] += src->linesize[i] * (src->height/2 - 1);
				src->linesize[i] = -src->linesize[i];
			}
		}
		else
		{
			if (!warnedAboutSlowFlip)
			{
				VUserLog("Unknown pixelformat %s (%d planes); using slow flipper.", av_get_pix_fmt_name((AVPixelFormat)src->format), planeCount);
				warnedAboutSlowFlip = true;
			}
			slowFlip = true;
		}

		int result = sws_scale(img_convert_ctx, src->data, src->linesize, 0, height, dst->data, dst->linesize);
		sws_freeContext(img_convert_ctx);

		if (slowFlip)
			FlipImageBytesVertical(dst->data[0], dst->linesize[0], height);

		return result;
	}

	/**
	 * Convert an AVFrame to a VuoImage.
	 */
	static VuoImage VuoImageWithAVFrame(AVCodecContext* videoCodecCtx, AVFrame* frame)
	{
		VuoImage image = NULL;
		AVPixelFormat pixelFormat = AV_PIX_FMT_RGBA;

		// Allocate an AVFrame structure
		AVFrame *pFrameRGB = av_frame_alloc();
		if (!pFrameRGB)
			return image;

		int linesize = av_image_get_linesize(pixelFormat, videoCodecCtx->width, 0);
		int numBytes = av_image_get_buffer_size(pixelFormat, videoCodecCtx->width, videoCodecCtx->height, linesize);
		uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

		av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, pixelFormat, videoCodecCtx->width, videoCodecCtx->height, 1);

		// Convert the image from its native format to RGB
		ConvertAVFrameToRGB(pFrameRGB, pixelFormat, frame, videoCodecCtx->pix_fmt, videoCodecCtx->width, videoCodecCtx->height);

		// Write frame to GL texture and create a VuoImage for it, then break and return.
		image = VuoImage_makeFromBuffer(pFrameRGB->data[0], GL_RGBA, videoCodecCtx->width, videoCodecCtx->height, VuoImageColorDepth_8, ^(void *){
			av_free(buffer);
			av_free(pFrameRGB);
		});

		return image;
	}
}

#pragma clang diagnostic pop

#ifdef __cplusplus
}
#endif
