/**
 * @file
 * VuoMovie implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */
#include "VuoMovie.h"
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <avcodec.h>
#include <avformat.h>
#include <avutil.h>
#include <swscale.h>
#pragma clang diagnostic pop

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMovie",
					 "dependencies" : [
						"avcodec",
						"avformat",
						"avutil",
						"swscale"
					 ],
					 "compatibleOperatingSystems": {
						 "macosx" : { "min": "10.6" }
					 }
				 });
#endif
}

#define SEC_PER_USEC .000001	///< Seconds per microsecond. Vuo nodes want information in seconds, where ffmpeg operates in microseconds.
#define REWIND .1				///< How many seconds behind the requested frame to seek.
#define REWIND_INCREMENT .2		///< If the REWIND value does not seek far enough, increment by this value.

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

		int videoStreamIndex;
		int64_t startPts;

		double duration;				// in seconds
		int packetDuration = 0;			// int time_base units

		int64_t lastTimestamp = 0;		// in microseconds
		int64_t lastPts = 0;			// in av units

//		int64_t *pts;					// a cache of the frame pts values - used when seeking
		int64_t firstFrame, lastFrame;

	}  AVContainer;

	/**
	 * Stores instance playback information.
	 */
	AVContainer container;

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

		container.videoStreamIndex = -1;

		for(int i = 0; i < (container.pFormatCtx)->nb_streams; i++)
			if( (container.pFormatCtx)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				container.videoStreamIndex = i;
				break;
			}

		if(container.videoStreamIndex < 0)
		  return -1; // Didn't find a video stream

		container.startPts = 0;//container.pFormatCtx->start_time;

		// Get a pointer to the codec context for the video stream
		container.video_st = (container.pFormatCtx)->streams[container.videoStreamIndex];
		container.pCodecCtx = container.video_st->codec;

		// Find the decoder for the video stream
		pCodec = avcodec_find_decoder(container.pCodecCtx->codec_id);

		if(pCodec == NULL)
		  return -1; // Codec not found

		// Open codec
		if(avcodec_open2(container.pCodecCtx, pCodec, NULL) < 0)
		  return -1; // Could not open codec

		nextFrame(NULL);
		container.firstFrame = container.lastPts;	// get the first frame's pts value, since format->start_pts isn't reliable

		container.duration = av_rescale_q ( container.video_st->duration,  container.video_st->time_base, AV_TIME_BASE_Q ) * SEC_PER_USEC;

		// seek function clamps desired frame timestamp to first and last pts, so set it super high on the first run
		container.lastFrame = INT64_MAX;

		// seek to 1 microsecond before projected duration (if a duration value was available - otherwise we've got to step from start)
		if(container.duration > 0.)
			seekToMs( (container.duration * 1000)-1000  );

		while(nextFrame(NULL))
			;

		container.lastFrame = container.lastPts;

		container.duration = av_rescale_q(container.lastFrame-container.firstFrame, container.video_st->time_base, AV_TIME_BASE_Q ) * SEC_PER_USEC;;

		seekFrame(0);

//		VLog("first frame pts: %lli last frame pts: %lli", container.firstFrame, container.lastFrame);
//		VLog("packetDuration says: %i", container.packetDuration);
//		VLog("second duration is %f", container.duration);
//		int64_t sec = secondToAvTime(container.duration);
//		VLog("sec %f to pts is %lli", container.duration, sec);
//		VLog("File: %s\nCodec: %s", path, pCodec->long_name);

		return 0;
	}

	/**
	 * Returns the last timestamp in seconds.
	 */
	double getCurrentSecond()
	{
		return container.lastTimestamp * SEC_PER_USEC;
	}

	/**
	 * Attempts to extract the frame image and timestamp for the next full frame in the current stream.
	 */
	bool getNextFrame(VuoImage *image, double *frameTimestampInSeconds)
	{
		bool gotNextFrame = nextFrame(image);

		if(gotNextFrame)
			*frameTimestampInSeconds = container.lastTimestamp * SEC_PER_USEC;

		return gotNextFrame;
	}

	/**
	 * Attempts to extract chronologically prior frame image and timestamp in the current stream.
	 */
	bool getPreviousFrame(VuoImage *image, double *frameTimestampInSeconds)
	{
		if( container.lastPts-container.packetDuration <= container.firstFrame || !seekFrame(container.lastPts - container.packetDuration) )
		{
			return false;
		}

		bool gotNextFrame = nextFrame(image);

		if(gotNextFrame)
			*frameTimestampInSeconds = container.lastTimestamp * SEC_PER_USEC;

		return gotNextFrame;
	}

	/**
	 * Returns a @c VuoList_VuoReal containing every frame's presentation time stamp.  In the event that timestamp information is not available, the current frame index multiplied by time base is used.
	 */
	VuoList_VuoReal extractFramePtsValues()
	{
		VuoList_VuoReal framePts = VuoListCreate_VuoReal();

		while(nextFrame(NULL))
			VuoListAppendValue_VuoReal(framePts, container.lastTimestamp);

		return framePts;
	}

	// http://stackoverflow.com/questions/5261658/how-to-seek-in-ffmpeg-c-c?lq=1
	/**
	 * Converts @a millisecond to frame PTS and performs an @c av_seek_frame() call.
	 */
	bool seekToMs(int64_t ms)
	{
		int64_t desiredFrameNumber = av_rescale(ms,
												container.pFormatCtx->streams[container.videoStreamIndex]->time_base.den,
												container.pFormatCtx->streams[container.videoStreamIndex]->time_base.num);
		desiredFrameNumber /= 1000;	// convert to microsecond

		return seekFrame(desiredFrameNumber);
	}

private:

	int64_t secondToAvTime(double sec)
	{
		int64_t scaled = av_rescale(sec *= 1000,
									container.pFormatCtx->streams[container.videoStreamIndex]->time_base.den,
									container.pFormatCtx->streams[container.videoStreamIndex]->time_base.num);
		scaled /= 1000;	// convert to microsecond
		return scaled;
	}

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

	double rewind = .1;
	double rewindIncrement = .2;
	bool seekFrame(int64_t frame)
	{
		if(frame > container.lastFrame)
			frame = container.lastFrame;

		if(frame < container.firstFrame)
			frame = container.firstFrame;

		avcodec_flush_buffers(container.pCodecCtx);

		int64_t seek = frame - secondToAvTime(rewind);

		int ret = av_seek_frame(container.pFormatCtx, container.videoStreamIndex, seek, AVSEEK_FLAG_ANY);

		container.lastPts = 0;

		if(ret < 0) {
			VLog("Failed seek - looking for frame pts: %lli", frame);
			return false;
		}

		if(frame <= container.firstFrame)
			return nextFrame(NULL);

		bool frameDecoded = false;

		while(container.lastPts + container.packetDuration < frame)
		{
			// don't bail on a null frame in this loop, since nextFrame() can sometimes
			// decode a junk frame and return false when there are in fact frames still
			// left.  test with `/System/Library/Compositions/Yosemite.mov` to see this
			// behavior.
			if( !nextFrame(NULL) )
			{
				container.lastPts = frame;
				if(frameDecoded)
					frame = container.lastPts;
			}
			else
				frameDecoded = true;

			// av_seek_frame can put the index before or after the desired frame.  when it overshoots,
			// seek to a further back point to guarantee we get a frame before and save the rewind
			// time used so that in future cycles this won't be necessary.
			if(container.lastPts >= frame)
			{
				avcodec_flush_buffers(container.pCodecCtx);
				rewind += rewindIncrement;
				ret = av_seek_frame(container.pFormatCtx, container.videoStreamIndex, frame-secondToAvTime(rewind), AVSEEK_FLAG_ANY);
				if(ret < 0)	return false;
				container.lastPts = container.firstFrame;
			}
		}

		return container.lastPts < frame;

		/// @todo (https://b33p.net/kosada/node/6598)
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
		image = VuoImage_makeFromBuffer(pFrameRGB->data[0], GL_RGBA, pCodecCtx->width, pCodecCtx->height);

		// Free the RGB image
		av_free(buffer);
		av_free(pFrameRGB);

		return image;
	}

	bool nextFrame(VuoImage *image)
	{
		AVFrame *pFrame = avcodec_alloc_frame();
		int frameFinished = 0;
		AVPacket packet;
		bool lastFrameOk = false;
		if (image != NULL)
			*image = NULL;

		//	@todo Refactor av_read_frame loop to it's own method, as it is called during a bunch of other tasks
		while(av_read_frame(container.pFormatCtx, &packet) >= 0)
		{
			// Is this a packet from the video stream?
			if(packet.stream_index == container.videoStreamIndex)
			{
				avcodec_decode_video2(container.pCodecCtx, pFrame, &frameFinished, &packet);

				// Did we get a video frame?
				if(frameFinished)
				{
					// Duration of this packet in AVStream->time_base units, 0 if unknown.
					container.packetDuration = packet.duration;

					// Get PTS here because formats with I frames can return junk values before a full frame is found
					int64_t pts = av_frame_get_best_effort_timestamp ( pFrame );
					container.lastPts = pts;

//					VLog("pts: %lli", pts);

					if( pts == AV_NOPTS_VALUE )
						pts = 1;//pts = container.frameCount * av_q2d(container.video_st->time_base);	// if no pts, guess using the time base and current frame count.
					else
						pts = av_rescale_q ( pts,  container.video_st->time_base, AV_TIME_BASE_Q );  // pts is now in microseconds.

					container.lastTimestamp = pts;

					if(image != NULL)
						*image = vuoImageWithAvFrame(container.pCodecCtx, pFrame);

					lastFrameOk = true;

					av_free_packet(&packet);

					break;
				}
			}
		}
		av_free(pFrame);
		return lastFrameOk;
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
VuoMovie VuoMovie_make(const char *path)
{
	VuoMovieDecoder *decoder = new VuoMovieDecoder();

	if (decoder->initWithFile(path) != 0)
	{
		VLog("Failed opening movie at path: \"%s\".", path);
		delete decoder;
		return NULL;
	}
	else
	{
		VuoRegister(decoder, VuoMovie_free);
		return (VuoMovie)decoder;
	}
}

/**
 * VuoMovie_getNextFrame gets the next full frame's image and presentation timestamp (in seconds).  Will return true if next frame is found and false if not.
 */
bool VuoMovie_getNextFrame(VuoMovie movie, VuoImage *image, double *nextFrame)
{
	return ((VuoMovieDecoder*)movie)->getNextFrame(image, nextFrame);
}

/**
 * Returns the frame prior to the internal currently queued frame, and the presentation timestamp associated.
 */
bool VuoMovie_getPreviousFrame(VuoMovie movie, VuoImage *image, double *previousFrame)
{
	return ((VuoMovieDecoder*)movie)->getPreviousFrame(image, previousFrame);
}

/**
 * Seeks the video to the specified second.
 */
bool VuoMovie_seekToSecond(VuoMovie movie, double second)
{
	// Accept a second to keep in tabs with rest of VuoMovie i/o, but convert to millisecond for use in ffmpeg (which will convert this to microsecond after applying time-base conversion)
	int64_t ms = second * 1000;	// now in milliseconds
	return ((VuoMovieDecoder*)movie)->seekToMs(ms);
}

/**
 * Returns the last successfully decoded timestamp.
 */
double VuoMovie_getCurrentSecond(VuoMovie movie)
{
	return ((VuoMovieDecoder*)movie)->getCurrentSecond();
}

/**
 * Returns the duration of the movie in seconds.
 */
double VuoMovie_getDuration(VuoMovie movie)
{
	return ((VuoMovieDecoder*)movie)->container.duration;
}

/**
 * Given a @a path, this will open and close a video stream and return the duration.
 */
bool VuoMovie_getInfo(const char *path, double *duration)
{
	VuoMovie movie = VuoMovie_make(path);
	if(movie == NULL)
		return false;

	double dur = ((VuoMovieDecoder*)movie)->container.pFormatCtx->duration * SEC_PER_USEC;
	*duration = dur > 0. ? dur : 0.;

	VuoRetain(movie);
	VuoRelease(movie);

	return true;
}
