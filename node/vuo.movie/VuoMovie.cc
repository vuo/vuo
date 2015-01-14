/**
 * @file
 * VuoMovie implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */
#include "VuoMovie.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoGlContext.h"
#include "VuoGlPool.h"
#include <time.h>
#include "VuoList_VuoReal.h"

extern "C"
{
#include "module.h"
#include <dispatch/dispatch.h>
#include <avcodec.h>
#include <avformat.h>
#include <avutil.h>
#include <swscale.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMovie",
					 "dependencies" : [
						"avcodec",
						"avformat",
						"avutil",
						"swscale",
						"VuoGlContext",
						"VuoGlPool",
						"OpenGL.framework",
						"CoreFoundation.framework"
					 ],
					 "compatibleOperatingSystems": {
						 "macosx" : { "min": "10.6" }
					 }
				 });
#endif
}

#define SEC_PER_USEC .000001 ///< Seconds per millisecond. Vuo nodes want information in seconds, where ffmpeg operates in milliseconds.

/**
 * Instance class used to control the playback of video.
 * \sa @c VuoMovie_getNextFrame @c VuoMovie_initFfmpeg @c VuoMovie_getInfo @c VuoMovie_make
 */
class VuoMovieDecoder
{
public:

	/**
	 * Internal struct which contains context and current playback status of VuoMovieDecoder.
	 */
	typedef struct AVContainer
	{
		AVFormatContext *pFormatCtx;
		AVCodecContext *pCodecCtx;

		AVStream *video_st;

		int videoStream;
		int64_t startPts;
		int totalFrames;

	}  AVContainer;

	/**
	 * Stores instance playback information.
	 */
	AVContainer container;

	/**
	 * The index of the last read frame.
	 */
	uint64_t frameCount = 0;

	/**
	 *	VuoMovieDecoder class destructor.
	 *	\sa @c VuoMovie_free
	 */
	~VuoMovieDecoder()
	{
		if(container.pCodecCtx != NULL)
			avcodec_close(container.pCodecCtx);

		if(container.pFormatCtx != NULL)
			avformat_close_input(&container.pFormatCtx);
	};

	/**
	 * Attempts to initialize an AVFormatContext and appropriate codec for the video @a path.
	 * \sa @c VuoMovie_make
	 */
	int initWithFile(const char *path)
	{
		// http://dranger.com/ffmpeg/tutorial01.html
		container.pFormatCtx = NULL;

		if(avformat_open_input(&(container.pFormatCtx), path, NULL, NULL) != 0)
			return -1; // Couldn't open file

		if(avformat_find_stream_info( container.pFormatCtx, NULL) < 0)
		  return -1; // Couldn't find stream information

		// Find the first video stream
		AVCodec *pCodec = NULL;
		container.pCodecCtx = NULL;

		container.videoStream = -1;

		for(int i = 0; i < (container.pFormatCtx)->nb_streams; i++)
			if( (container.pFormatCtx)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				container.videoStream = i;
				break;
			}

		if(container.videoStream < 0)
		  return -1; // Didn't find a video stream

		container.totalFrames = container.pFormatCtx->streams[container.videoStream]->nb_frames;
		container.startPts = container.pFormatCtx->start_time;

		// Get a pointer to the codec context for the video stream
		container.video_st = (container.pFormatCtx)->streams[container.videoStream];
		container.pCodecCtx = container.video_st->codec;

		// Find the decoder for the video stream
		pCodec = avcodec_find_decoder(container.pCodecCtx->codec_id);

		if(pCodec == NULL)
		  return -1; // Codec not found

		// Open codec
		if(avcodec_open2(container.pCodecCtx, pCodec, NULL) < 0)
		  return -1; // Could not open codec

//		VLog("File: %s\nCodec: %s", path, pCodec->long_name);

		return 0;
	}


	/**
	 * Attempts to extract the frame image and timestamp for the next full frame in the current stream.
	 */
	bool getNextFrame(VuoImage *image, double *nextFrame)
	{
		AVFrame *pFrame = NULL;

		// Allocate video frame
		pFrame = avcodec_alloc_frame();

		int frameFinished = 0;
		AVPacket packet;

		// @todo Refactor av_read_frame loop to it's own method, as it is called during a bunch of other tasks
		while(av_read_frame(container.pFormatCtx, &packet) >= 0)
		{
			// Is this a packet from the video stream?
			if(packet.stream_index == container.videoStream)
			{
				// Decode video frame
				avcodec_decode_video2(container.pCodecCtx, pFrame, &frameFinished, &packet);

				// Did we get a video frame?
				if(frameFinished)
				{
					// Get PTS here because formats with I frames can return junk values before a full frame is found
					int64_t pts = av_frame_get_best_effort_timestamp ( pFrame );

					if( pts == AV_NOPTS_VALUE )
						pts = frameCount * av_q2d(container.video_st->time_base);	// if no pts, guess using the time base and current frame count.
					else
						pts = av_rescale_q ( pts,  container.video_st->time_base, AV_TIME_BASE_Q );  // pts is now in microseconds.

					*image = vuoImageWithAvFrame(container.pCodecCtx, pFrame);
					*nextFrame = pts * SEC_PER_USEC;

					lastTimestamp = pts;

					av_free_packet(&packet);

					frameCount++;
					break;
				}
			}

			// Free the packet that was allocated by av_read_frame
		}

		// Free the YUV frame
		av_free(pFrame);

		lastFrameOk = frameFinished;

		return frameFinished != 0;
	}

	/**
	 * Returns a @c VuoList_VuoReal containing every frame's presentation time stamp.  In the event that timestamp information is not available, the current frame index multiplied by time base is used.
	 */
	VuoList_VuoReal extractFramePtsValues()
	{
		VuoList_VuoReal framePts = VuoListCreate_VuoReal();
		AVPacket packet;
		AVFrame *pFrame = NULL;
		// Allocate video frame
		pFrame = avcodec_alloc_frame();
		int frameFinished;

		while(av_read_frame(container.pFormatCtx, &packet) >= 0)
		{
			// Is this a packet from the video stream?
			if(packet.stream_index == container.videoStream)
			{
				// Decode video frame
				avcodec_decode_video2(container.pCodecCtx, pFrame, &frameFinished, &packet);

				if(frameFinished)
				{
					int64_t pts = av_frame_get_best_effort_timestamp ( pFrame );

					if( pts == AV_NOPTS_VALUE )
						pts = frameCount * av_q2d(container.video_st->time_base);	// if no pts, guess using the time base and current frame count.
					else
						pts = av_rescale_q ( pts,  container.video_st->time_base, AV_TIME_BASE_Q );  // pts is now in microseconds.

					VuoListAppendValue_VuoReal(framePts, pts * SEC_PER_USEC);
				}
			}
		}
		return framePts;
	}

	// http://stackoverflow.com/questions/5261658/how-to-seek-in-ffmpeg-c-c?lq=1
	/**
	 * Converts @a millisecond to frame PTS and performs an @c av_seek_frame() call.
	 */
	bool seekToMs(int64_t ms)
	{
		ms += container.startPts;	// video start time is not guaranteed to be 0, or so ffmpeg docs say
		int64_t desiredFrameNumber = av_rescale(ms,
												container.pFormatCtx->streams[container.videoStream]->time_base.den,
												container.pFormatCtx->streams[container.videoStream]->time_base.num);
		desiredFrameNumber /= 1000;	// convert to microsecond

		return seekFrame(desiredFrameNumber);
	}

private:

	int64_t lastTimestamp;
	bool lastFrameOk;

	int img_convert(AVPicture* dst, PixelFormat dst_pix_fmt, AVPicture* src, PixelFormat pix_fmt, int width, int height)
	{
		int av_log = av_log_get_level();
		av_log_set_level(AV_LOG_QUIET);
		SwsContext *img_convert_ctx = sws_getContext(width, height, pix_fmt, width, height, dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

		int result = sws_scale(img_convert_ctx, src->data, src->linesize, 0, height, dst->data, dst->linesize);
		sws_freeContext(img_convert_ctx);
		av_log_set_level(av_log);

		// Flip the image vertically, because otherwise glTexImage2d() reads in the pixels upside down.
		// AVFrame->data[0] is stored like so : [ row 0 ] [ row 1 ] [ row 2 ] [ etc ]
		int lines = height;
		uint stride = dst->linesize[0];
		uint size = stride * sizeof(uint8_t);
		uint8_t *tmp = (uint8_t*)malloc(size);
		for(int i = 0; i < lines/2; i++)
		{
			memcpy(tmp, dst->data[0] + stride*i, size);
			memcpy(dst->data[0]+stride*i, dst->data[0]+stride*(lines-i-1), size);
			memcpy(dst->data[0]+stride*(lines-i-1), tmp, size);
		}
		free(tmp);

		return result;
	}

	bool seekFrame(int64_t frame)
	{
		avcodec_flush_buffers(container.pCodecCtx);
		int ret = av_seek_frame(container.pFormatCtx, container.videoStream, frame, AVSEEK_FLAG_ANY);
		return ret >= 0;


		/// @todo (https://b33p.net/kosada/node/6598)

	   //printf("**** seekFrame to %d. LLT: %d. LT: %d. LLF: %d. LF: %d. LastFrameOk: %d\n",(int)frame,LastLastFrameTime,LastFrameTime,LastLastFrameNumber,LastFrameNumber,(int)LastFrameOk);

		// Seek if:
	   // - we don't know where we are (Ok=false)
	   // - we know where we are but:
	   //    - the desired frame is after the last decoded frame (this could be optimized: if the distance is small, calling decodeSeekFrame may be faster than seeking from the last key frame)
	   //    - the desired frame is smaller or equal than the previous to the last decoded frame. Equal because if frame==LastLastFrameNumber we don't want the LastFrame, but the one before->we need to seek there

		// If last frame was borked, or the desired frame is before the last read frame, seek.
		if(!lastFrameOk || (lastFrameOk && frame > lastTimestamp))
		{
			if(av_seek_frame(container.pFormatCtx, container.videoStream, frame, (frame < lastTimestamp ? AVSEEK_FLAG_BACKWARD : AVSEEK_FLAG_ANY)) < 0)
				return false;
			else
				lastTimestamp = frame-1;

			avcodec_flush_buffers(container.pCodecCtx);
		}

		AVPacket packet;
		AVFrame *pFrame = NULL;
		pFrame = avcodec_alloc_frame();

		int frameFinished = 0;

		while(av_read_frame(container.pFormatCtx, &packet) >= 0)
		{
			// Is this a packet from the video stream?
			if(packet.stream_index == container.videoStream)
			{
				// Decode video frame
				avcodec_decode_video2(container.pCodecCtx, pFrame, &frameFinished, &packet);

				// Did we get a video frame?
				if(frameFinished)
				{
					// Get PTS here because formats with I frames can return junk values before a full frame is found
					int64_t pts = av_frame_get_best_effort_timestamp ( pFrame );

					if( pts == AV_NOPTS_VALUE )
					{
						/// todo - Remove after testing vuo video
						continue;
//						pts = frameCount * av_q2d(container.video_st->time_base);	// if no pts, guess using the time base and current frame count.
					}

					lastTimestamp = pts;

					 if(lastTimestamp >= frame)
						 break;
				}
			}
		}
	   return true;
	}

	VuoImage vuoImageWithAvFrame(AVCodecContext *pCodecCtx, AVFrame* pFrame)
	{
		VuoImage image = NULL;
		AVPixelFormat pixelFormat = PIX_FMT_RGBA;

		// Allocate an AVFrame structure
		AVFrame *pFrameRGB = NULL;
		pFrameRGB = avcodec_alloc_frame();

		if(pFrameRGB == NULL)
			return image;

		uint8_t *buffer = NULL;
		int numBytes;

		numBytes = avpicture_get_size(pixelFormat,
									  pCodecCtx->width,
									  pCodecCtx->height);

		buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

		avpicture_fill( (AVPicture *)pFrameRGB, buffer, pixelFormat,
						pCodecCtx->width,
						pCodecCtx->height);

		// Convert the image from its native format to RGB
		img_convert((AVPicture *)pFrameRGB, pixelFormat, (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

		// Write frame to GL texture and create a VuoImage for it, then break and return.
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
		{
			GLuint textureid = VuoGlPool_use(VuoGlPool_Texture);
			glBindTexture(GL_TEXTURE_2D, textureid);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pCodecCtx->width, pCodecCtx->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pFrameRGB->data[0]);

			glBindTexture(GL_TEXTURE_2D, 0);

			image = VuoImage_make(textureid, pCodecCtx->width, pCodecCtx->height);
		}

		VuoGlContext_disuse(cgl_ctx);

		// Free the RGB image
		av_free(buffer);
		av_free(pFrameRGB);

		return image;
	}
};

/**
 * Initializes the FFMPEG libraries.
 */
static void __attribute__((constructor)) VuoMovie_initFfmpeg(void)
{
	av_register_all();
	avformat_network_init();
}

/**
 * Releases ffmpeg resources created during this object's lifecycle.
 */
void VuoMovie_free(void *movie)
{
	/// @todo Close movie file and free memory. (https://b33p.net/kosada/node/6595)
	delete (VuoMovieDecoder*)movie;
}

/**
 * @brief VuoMovie_make: Creates a new VuoMovieDecoder object and returns itself.
 * @param path An absolute path to the video file to open.
 * @return A new VuoMovie object, which may be used to control playback of the film.  If the video file fails to open for any reason, this returns NULL.
 */
VuoMovie * VuoMovie_make(const char *path)
{
	VuoMovieDecoder *movie = new VuoMovieDecoder();

	if(movie->initWithFile(path) != 0 )
	{
		VLog("Failed opening movie at path: \"%s\".", path);
		delete movie;
		return NULL;
	}
	else
	{
		VuoRegister(movie, VuoMovie_free);
		return (VuoMovie*)movie;
	}
}

/**
 * VuoMovie_getNextFrame gets the next full frame's image and presentation timestamp (in seconds).  Will return true if next frame is found and false if not.
 */
bool VuoMovie_getNextFrame(VuoMovie *decoder, VuoImage *image, double *nextFrame)
{
	return ((VuoMovieDecoder*)decoder)->getNextFrame(image, nextFrame);
}

/**
 * Seeks the video to the specified second.
 */
bool VuoMovie_seekToSecond(VuoMovie *movie, double second)
{
	// Accept a second to keep in tabs with rest of VuoMovie i/o, but convert to millisecond for use in ffmpeg (which will convert this to microsecond after applying time-base conversion)
	int64_t ms = second * 1000;	// now in milliseconds
	return ((VuoMovieDecoder*)movie)->seekToMs(ms);
}

/**
 * Given a @a path, this will open and close a video stream and return the duration.
 */
bool VuoMovie_getInfo(const char *path, double *duration)
{
	VuoMovie *decoder = VuoMovie_make(path);
	if(decoder == NULL)
		return false;

	double dur = ((VuoMovieDecoder*)decoder)->container.pFormatCtx->duration * SEC_PER_USEC;
	*duration = dur > 0. ? dur : 0.;

	VuoMovie_free(decoder);

	return true;
}
