/**
 * @file
 * VuoMovie implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
#include <string.h>
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
		char *path;

		AVFormatContext *pFormatCtx;
		AVCodecContext *pCodecCtx;

		AVStream *video_st;

		int videoStreamIndex;
		int64_t startPts;

		double duration;				// in seconds
		int packetDuration = 0;			// int time_base units
		int previousPacketDuration = 0;	// the duration of the packet immediately prior to the last queued packet.

		int64_t lastTimestamp = 0;		// in microseconds
		int64_t lastPts = 0;			// in av units

//		int64_t *pts;					// a cache of the frame pts values - used when seeking
		int64_t firstFrame, lastFrame;	// the first and last frame pts values

		bool seekUnavailable;			// if true, the seek function will close and re-open it's stream
										// when seeking, then step to the desired frame.  necessary with
										// some codecs; namely flv and some gif files
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
		container.pFormatCtx = NULL;
		if(avformat_open_input(&(container.pFormatCtx), path, NULL, NULL) != 0)
		{
			VLog("Error: Could not find path: \"%s\"", path);
			return -1; // Couldn't open file
		}

		container.path = (char*)path;//(char*)malloc(sizeof(path));

		// initialize the codecs and context stuff
		if( initCodecCtx() < 0 )
			return -1;

		// get more accurate duration
		nextFrame(NULL);
		container.firstFrame = container.lastPts;	// get the first frame's pts value, since format->start_pts isn't reliable

		container.duration = av_rescale_q ( container.video_st->duration,  container.video_st->time_base, AV_TIME_BASE_Q ) * SEC_PER_USEC;

		// seek function clamps desired frame timestamp to first and last pts, so set it super high on the first run
		container.lastFrame = INT64_MAX;

		// seek to 1 microsecond before projected duration (if a duration value was available - otherwise we've got to step from start)
		// codecs like flash and some gif don't get seek support from ffmpeg, so do things the hard way.
		if(container.duration > 0. && !container.seekUnavailable)
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
//		VLog("File: %s\nCodec: %s", path, container.pCodecCtx->codec_name);

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
		if( container.lastPts-container.previousPacketDuration <= container.firstFrame || !seekFrame(container.lastPts - container.previousPacketDuration) )
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

	int initCodecCtx()
	{
		// http://dranger.com/ffmpeg/tutorial01.html

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

		if (pCodec == NULL)
		  return -1; // Codec not found

//		VLog("Codec: %s", AVCodecIDToString(pCodec->id) );

		// Flash can't seek, so when seeking just jet to 0
		if(pCodec->id == AV_CODEC_ID_FLV1 || pCodec->id == AV_CODEC_ID_GIF)
			container.seekUnavailable = true;

		// Open codec
		if(avcodec_open2(container.pCodecCtx, pCodec, NULL) < 0)
		  return -1; // Could not open codec

		return 0;
	}

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

	double rewind = .05;
	double rewindIncrement = .1;
	bool seekFrame(int64_t frame)
	{
//		VLog("SeekTo: %lli", frame);

		if(frame > container.lastFrame)
			frame = container.lastFrame;

		if(frame < container.firstFrame)
			frame = container.firstFrame;

		avcodec_flush_buffers(container.pCodecCtx);

		int64_t seek = frame - secondToAvTime(rewind);

		int ret = av_seek_frame(container.pFormatCtx, container.videoStreamIndex, seek, AVSEEK_FLAG_ANY);

		container.lastPts = 0;

		if(ret < 0)
		{
			if(container.seekUnavailable)
			{
				if(container.pCodecCtx != NULL)
					avcodec_close(container.pCodecCtx);

				if(container.pFormatCtx != NULL)
					avformat_close_input(&container.pFormatCtx);

				if(avformat_open_input(&(container.pFormatCtx), container.path, NULL, NULL) != 0)
					return false;

				initCodecCtx();
			}
			else
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
				if(ret < 0) return false;

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
					container.previousPacketDuration = container.packetDuration;
					container.packetDuration = packet.duration;

					// Get PTS here because formats with I frames can return junk values before a full frame is found
					int64_t pts = av_frame_get_best_effort_timestamp ( pFrame );
					container.lastPts = pts;

//					VLog("pts: %lli  duration: %i", pts, container.packetDuration);

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

	char* AVCodecIDToString(AVCodecID id)
	{
		if(id == AV_CODEC_ID_NONE)
		   return (char*)"AV_CODEC_ID_NONE";
		else
		if(id == AV_CODEC_ID_MPEG1VIDEO)
		   return (char*)"AV_CODEC_ID_MPEG1VIDEO";
		else
		if(id == AV_CODEC_ID_MPEG2VIDEO)
		   return (char*)"AV_CODEC_ID_MPEG2VIDEO";
		else
		if(id == AV_CODEC_ID_H261)
		   return (char*)"AV_CODEC_ID_H261";
		else
		if(id == AV_CODEC_ID_H263)
		   return (char*)"AV_CODEC_ID_H263";
		else
		if(id == AV_CODEC_ID_RV10)
		   return (char*)"AV_CODEC_ID_RV10";
		else
		if(id == AV_CODEC_ID_RV20)
		   return (char*)"AV_CODEC_ID_RV20";
		else
		if(id == AV_CODEC_ID_MJPEG)
		   return (char*)"AV_CODEC_ID_MJPEG";
		else
		if(id == AV_CODEC_ID_MJPEGB)
		   return (char*)"AV_CODEC_ID_MJPEGB";
		else
		if(id == AV_CODEC_ID_LJPEG)
		   return (char*)"AV_CODEC_ID_LJPEG";
		else
		if(id == AV_CODEC_ID_SP5X)
		   return (char*)"AV_CODEC_ID_SP5X";
		else
		if(id == AV_CODEC_ID_JPEGLS)
		   return (char*)"AV_CODEC_ID_JPEGLS";
		else
		if(id == AV_CODEC_ID_MPEG4)
		   return (char*)"AV_CODEC_ID_MPEG4";
		else
		if(id == AV_CODEC_ID_RAWVIDEO)
		   return (char*)"AV_CODEC_ID_RAWVIDEO";
		else
		if(id == AV_CODEC_ID_MSMPEG4V1)
		   return (char*)"AV_CODEC_ID_MSMPEG4V1";
		else
		if(id == AV_CODEC_ID_MSMPEG4V2)
		   return (char*)"AV_CODEC_ID_MSMPEG4V2";
		else
		if(id == AV_CODEC_ID_MSMPEG4V3)
		   return (char*)"AV_CODEC_ID_MSMPEG4V3";
		else
		if(id == AV_CODEC_ID_WMV1)
		   return (char*)"AV_CODEC_ID_WMV1";
		else
		if(id == AV_CODEC_ID_WMV2)
		   return (char*)"AV_CODEC_ID_WMV2";
		else
		if(id == AV_CODEC_ID_H263P)
		   return (char*)"AV_CODEC_ID_H263P";
		else
		if(id == AV_CODEC_ID_H263I)
		   return (char*)"AV_CODEC_ID_H263I";
		else
		if(id == AV_CODEC_ID_FLV1)
		   return (char*)"AV_CODEC_ID_FLV1";
		else
		if(id == AV_CODEC_ID_SVQ1)
		   return (char*)"AV_CODEC_ID_SVQ1";
		else
		if(id == AV_CODEC_ID_SVQ3)
		   return (char*)"AV_CODEC_ID_SVQ3";
		else
		if(id == AV_CODEC_ID_DVVIDEO)
		   return (char*)"AV_CODEC_ID_DVVIDEO";
		else
		if(id == AV_CODEC_ID_HUFFYUV)
		   return (char*)"AV_CODEC_ID_HUFFYUV";
		else
		if(id == AV_CODEC_ID_CYUV)
		   return (char*)"AV_CODEC_ID_CYUV";
		else
		if(id == AV_CODEC_ID_H264)
		   return (char*)"AV_CODEC_ID_H264";
		else
		if(id == AV_CODEC_ID_INDEO3)
		   return (char*)"AV_CODEC_ID_INDEO3";
		else
		if(id == AV_CODEC_ID_VP3)
		   return (char*)"AV_CODEC_ID_VP3";
		else
		if(id == AV_CODEC_ID_THEORA)
		   return (char*)"AV_CODEC_ID_THEORA";
		else
		if(id == AV_CODEC_ID_ASV1)
		   return (char*)"AV_CODEC_ID_ASV1";
		else
		if(id == AV_CODEC_ID_ASV2)
		   return (char*)"AV_CODEC_ID_ASV2";
		else
		if(id == AV_CODEC_ID_FFV1)
		   return (char*)"AV_CODEC_ID_FFV1";
		else
		if(id == AV_CODEC_ID_4XM)
		   return (char*)"AV_CODEC_ID_4XM";
		else
		if(id == AV_CODEC_ID_VCR1)
		   return (char*)"AV_CODEC_ID_VCR1";
		else
		if(id == AV_CODEC_ID_CLJR)
		   return (char*)"AV_CODEC_ID_CLJR";
		else
		if(id == AV_CODEC_ID_MDEC)
		   return (char*)"AV_CODEC_ID_MDEC";
		else
		if(id == AV_CODEC_ID_ROQ)
		   return (char*)"AV_CODEC_ID_ROQ";
		else
		if(id == AV_CODEC_ID_INTERPLAY_VIDEO)
		   return (char*)"AV_CODEC_ID_INTERPLAY_VIDEO";
		else
		if(id == AV_CODEC_ID_XAN_WC3)
		   return (char*)"AV_CODEC_ID_XAN_WC3";
		else
		if(id == AV_CODEC_ID_XAN_WC4)
		   return (char*)"AV_CODEC_ID_XAN_WC4";
		else
		if(id == AV_CODEC_ID_RPZA)
		   return (char*)"AV_CODEC_ID_RPZA";
		else
		if(id == AV_CODEC_ID_CINEPAK)
		   return (char*)"AV_CODEC_ID_CINEPAK";
		else
		if(id == AV_CODEC_ID_WS_VQA)
		   return (char*)"AV_CODEC_ID_WS_VQA";
		else
		if(id == AV_CODEC_ID_MSRLE)
		   return (char*)"AV_CODEC_ID_MSRLE";
		else
		if(id == AV_CODEC_ID_MSVIDEO1)
		   return (char*)"AV_CODEC_ID_MSVIDEO1";
		else
		if(id == AV_CODEC_ID_IDCIN)
		   return (char*)"AV_CODEC_ID_IDCIN";
		else
		if(id == AV_CODEC_ID_8BPS)
		   return (char*)"AV_CODEC_ID_8BPS";
		else
		if(id == AV_CODEC_ID_SMC)
		   return (char*)"AV_CODEC_ID_SMC";
		else
		if(id == AV_CODEC_ID_FLIC)
		   return (char*)"AV_CODEC_ID_FLIC";
		else
		if(id == AV_CODEC_ID_TRUEMOTION1)
		   return (char*)"AV_CODEC_ID_TRUEMOTION1";
		else
		if(id == AV_CODEC_ID_VMDVIDEO)
		   return (char*)"AV_CODEC_ID_VMDVIDEO";
		else
		if(id == AV_CODEC_ID_MSZH)
		   return (char*)"AV_CODEC_ID_MSZH";
		else
		if(id == AV_CODEC_ID_ZLIB)
		   return (char*)"AV_CODEC_ID_ZLIB";
		else
		if(id == AV_CODEC_ID_QTRLE)
		   return (char*)"AV_CODEC_ID_QTRLE";
		else
		if(id == AV_CODEC_ID_TSCC)
		   return (char*)"AV_CODEC_ID_TSCC";
		else
		if(id == AV_CODEC_ID_ULTI)
		   return (char*)"AV_CODEC_ID_ULTI";
		else
		if(id == AV_CODEC_ID_QDRAW)
		   return (char*)"AV_CODEC_ID_QDRAW";
		else
		if(id == AV_CODEC_ID_VIXL)
		   return (char*)"AV_CODEC_ID_VIXL";
		else
		if(id == AV_CODEC_ID_QPEG)
		   return (char*)"AV_CODEC_ID_QPEG";
		else
		if(id == AV_CODEC_ID_PNG)
		   return (char*)"AV_CODEC_ID_PNG";
		else
		if(id == AV_CODEC_ID_PPM)
		   return (char*)"AV_CODEC_ID_PPM";
		else
		if(id == AV_CODEC_ID_PBM)
		   return (char*)"AV_CODEC_ID_PBM";
		else
		if(id == AV_CODEC_ID_PGM)
		   return (char*)"AV_CODEC_ID_PGM";
		else
		if(id == AV_CODEC_ID_PGMYUV)
		   return (char*)"AV_CODEC_ID_PGMYUV";
		else
		if(id == AV_CODEC_ID_PAM)
		   return (char*)"AV_CODEC_ID_PAM";
		else
		if(id == AV_CODEC_ID_FFVHUFF)
		   return (char*)"AV_CODEC_ID_FFVHUFF";
		else
		if(id == AV_CODEC_ID_RV30)
		   return (char*)"AV_CODEC_ID_RV30";
		else
		if(id == AV_CODEC_ID_RV40)
		   return (char*)"AV_CODEC_ID_RV40";
		else
		if(id == AV_CODEC_ID_VC1)
		   return (char*)"AV_CODEC_ID_VC1";
		else
		if(id == AV_CODEC_ID_WMV3)
		   return (char*)"AV_CODEC_ID_WMV3";
		else
		if(id == AV_CODEC_ID_LOCO)
		   return (char*)"AV_CODEC_ID_LOCO";
		else
		if(id == AV_CODEC_ID_WNV1)
		   return (char*)"AV_CODEC_ID_WNV1";
		else
		if(id == AV_CODEC_ID_AASC)
		   return (char*)"AV_CODEC_ID_AASC";
		else
		if(id == AV_CODEC_ID_INDEO2)
		   return (char*)"AV_CODEC_ID_INDEO2";
		else
		if(id == AV_CODEC_ID_FRAPS)
		   return (char*)"AV_CODEC_ID_FRAPS";
		else
		if(id == AV_CODEC_ID_TRUEMOTION2)
		   return (char*)"AV_CODEC_ID_TRUEMOTION2";
		else
		if(id == AV_CODEC_ID_BMP)
		   return (char*)"AV_CODEC_ID_BMP";
		else
		if(id == AV_CODEC_ID_CSCD)
		   return (char*)"AV_CODEC_ID_CSCD";
		else
		if(id == AV_CODEC_ID_MMVIDEO)
		   return (char*)"AV_CODEC_ID_MMVIDEO";
		else
		if(id == AV_CODEC_ID_ZMBV)
		   return (char*)"AV_CODEC_ID_ZMBV";
		else
		if(id == AV_CODEC_ID_AVS)
		   return (char*)"AV_CODEC_ID_AVS";
		else
		if(id == AV_CODEC_ID_SMACKVIDEO)
		   return (char*)"AV_CODEC_ID_SMACKVIDEO";
		else
		if(id == AV_CODEC_ID_NUV)
		   return (char*)"AV_CODEC_ID_NUV";
		else
		if(id == AV_CODEC_ID_KMVC)
		   return (char*)"AV_CODEC_ID_KMVC";
		else
		if(id == AV_CODEC_ID_FLASHSV)
		   return (char*)"AV_CODEC_ID_FLASHSV";
		else
		if(id == AV_CODEC_ID_CAVS)
		   return (char*)"AV_CODEC_ID_CAVS";
		else
		if(id == AV_CODEC_ID_JPEG2000)
		   return (char*)"AV_CODEC_ID_JPEG2000";
		else
		if(id == AV_CODEC_ID_VMNC)
		   return (char*)"AV_CODEC_ID_VMNC";
		else
		if(id == AV_CODEC_ID_VP5)
		   return (char*)"AV_CODEC_ID_VP5";
		else
		if(id == AV_CODEC_ID_VP6)
		   return (char*)"AV_CODEC_ID_VP6";
		else
		if(id == AV_CODEC_ID_VP6F)
		   return (char*)"AV_CODEC_ID_VP6F";
		else
		if(id == AV_CODEC_ID_TARGA)
		   return (char*)"AV_CODEC_ID_TARGA";
		else
		if(id == AV_CODEC_ID_DSICINVIDEO)
		   return (char*)"AV_CODEC_ID_DSICINVIDEO";
		else
		if(id == AV_CODEC_ID_TIERTEXSEQVIDEO)
		   return (char*)"AV_CODEC_ID_TIERTEXSEQVIDEO";
		else
		if(id == AV_CODEC_ID_TIFF)
		   return (char*)"AV_CODEC_ID_TIFF";
		else
		if(id == AV_CODEC_ID_GIF)
		   return (char*)"AV_CODEC_ID_GIF";
		else
		if(id == AV_CODEC_ID_DXA)
		   return (char*)"AV_CODEC_ID_DXA";
		else
		if(id == AV_CODEC_ID_DNXHD)
		   return (char*)"AV_CODEC_ID_DNXHD";
		else
		if(id == AV_CODEC_ID_THP)
		   return (char*)"AV_CODEC_ID_THP";
		else
		if(id == AV_CODEC_ID_SGI)
		   return (char*)"AV_CODEC_ID_SGI";
		else
		if(id == AV_CODEC_ID_C93)
		   return (char*)"AV_CODEC_ID_C93";
		else
		if(id == AV_CODEC_ID_BETHSOFTVID)
		   return (char*)"AV_CODEC_ID_BETHSOFTVID";
		else
		if(id == AV_CODEC_ID_PTX)
		   return (char*)"AV_CODEC_ID_PTX";
		else
		if(id == AV_CODEC_ID_TXD)
		   return (char*)"AV_CODEC_ID_TXD";
		else
		if(id == AV_CODEC_ID_VP6A)
		   return (char*)"AV_CODEC_ID_VP6A";
		else
		if(id == AV_CODEC_ID_AMV)
		   return (char*)"AV_CODEC_ID_AMV";
		else
		if(id == AV_CODEC_ID_VB)
		   return (char*)"AV_CODEC_ID_VB";
		else
		if(id == AV_CODEC_ID_PCX)
		   return (char*)"AV_CODEC_ID_PCX";
		else
		if(id == AV_CODEC_ID_SUNRAST)
		   return (char*)"AV_CODEC_ID_SUNRAST";
		else
		if(id == AV_CODEC_ID_INDEO4)
		   return (char*)"AV_CODEC_ID_INDEO4";
		else
		if(id == AV_CODEC_ID_INDEO5)
		   return (char*)"AV_CODEC_ID_INDEO5";
		else
		if(id == AV_CODEC_ID_MIMIC)
		   return (char*)"AV_CODEC_ID_MIMIC";
		else
		if(id == AV_CODEC_ID_RL2)
		   return (char*)"AV_CODEC_ID_RL2";
		else
		if(id == AV_CODEC_ID_ESCAPE124)
		   return (char*)"AV_CODEC_ID_ESCAPE124";
		else
		if(id == AV_CODEC_ID_DIRAC)
		   return (char*)"AV_CODEC_ID_DIRAC";
		else
		if(id == AV_CODEC_ID_BFI)
		   return (char*)"AV_CODEC_ID_BFI";
		else
		if(id == AV_CODEC_ID_CMV)
		   return (char*)"AV_CODEC_ID_CMV";
		else
		if(id == AV_CODEC_ID_MOTIONPIXELS)
		   return (char*)"AV_CODEC_ID_MOTIONPIXELS";
		else
		if(id == AV_CODEC_ID_TGV)
		   return (char*)"AV_CODEC_ID_TGV";
		else
		if(id == AV_CODEC_ID_TGQ)
		   return (char*)"AV_CODEC_ID_TGQ";
		else
		if(id == AV_CODEC_ID_TQI)
		   return (char*)"AV_CODEC_ID_TQI";
		else
		if(id == AV_CODEC_ID_AURA)
		   return (char*)"AV_CODEC_ID_AURA";
		else
		if(id == AV_CODEC_ID_AURA2)
		   return (char*)"AV_CODEC_ID_AURA2";
		else
		if(id == AV_CODEC_ID_V210X)
		   return (char*)"AV_CODEC_ID_V210X";
		else
		if(id == AV_CODEC_ID_TMV)
		   return (char*)"AV_CODEC_ID_TMV";
		else
		if(id == AV_CODEC_ID_V210)
		   return (char*)"AV_CODEC_ID_V210";
		else
		if(id == AV_CODEC_ID_DPX)
		   return (char*)"AV_CODEC_ID_DPX";
		else
		if(id == AV_CODEC_ID_MAD)
		   return (char*)"AV_CODEC_ID_MAD";
		else
		if(id == AV_CODEC_ID_FRWU)
		   return (char*)"AV_CODEC_ID_FRWU";
		else
		if(id == AV_CODEC_ID_FLASHSV2)
		   return (char*)"AV_CODEC_ID_FLASHSV2";
		else
		if(id == AV_CODEC_ID_CDGRAPHICS)
		   return (char*)"AV_CODEC_ID_CDGRAPHICS";
		else
		if(id == AV_CODEC_ID_R210)
		   return (char*)"AV_CODEC_ID_R210";
		else
		if(id == AV_CODEC_ID_ANM)
		   return (char*)"AV_CODEC_ID_ANM";
		else
		if(id == AV_CODEC_ID_BINKVIDEO)
		   return (char*)"AV_CODEC_ID_BINKVIDEO";
		else
		if(id == AV_CODEC_ID_IFF_ILBM)
		   return (char*)"AV_CODEC_ID_IFF_ILBM";
		else
		if(id == AV_CODEC_ID_IFF_BYTERUN1)
		   return (char*)"AV_CODEC_ID_IFF_BYTERUN1";
		else
		if(id == AV_CODEC_ID_KGV1)
		   return (char*)"AV_CODEC_ID_KGV1";
		else
		if(id == AV_CODEC_ID_YOP)
		   return (char*)"AV_CODEC_ID_YOP";
		else
		if(id == AV_CODEC_ID_VP8)
		   return (char*)"AV_CODEC_ID_VP8";
		else
		if(id == AV_CODEC_ID_PICTOR)
		   return (char*)"AV_CODEC_ID_PICTOR";
		else
		if(id == AV_CODEC_ID_ANSI)
		   return (char*)"AV_CODEC_ID_ANSI";
		else
		if(id == AV_CODEC_ID_A64_MULTI)
		   return (char*)"AV_CODEC_ID_A64_MULTI";
		else
		if(id == AV_CODEC_ID_A64_MULTI5)
		   return (char*)"AV_CODEC_ID_A64_MULTI5";
		else
		if(id == AV_CODEC_ID_R10K)
		   return (char*)"AV_CODEC_ID_R10K";
		else
		if(id == AV_CODEC_ID_MXPEG)
		   return (char*)"AV_CODEC_ID_MXPEG";
		else
		if(id == AV_CODEC_ID_LAGARITH)
		   return (char*)"AV_CODEC_ID_LAGARITH";
		else
		if(id == AV_CODEC_ID_PRORES)
		   return (char*)"AV_CODEC_ID_PRORES";
		else
		if(id == AV_CODEC_ID_JV)
		   return (char*)"AV_CODEC_ID_JV";
		else
		if(id == AV_CODEC_ID_DFA)
		   return (char*)"AV_CODEC_ID_DFA";
		else
		if(id == AV_CODEC_ID_WMV3IMAGE)
		   return (char*)"AV_CODEC_ID_WMV3IMAGE";
		else
		if(id == AV_CODEC_ID_VC1IMAGE)
		   return (char*)"AV_CODEC_ID_VC1IMAGE";
		else
		if(id == AV_CODEC_ID_UTVIDEO)
		   return (char*)"AV_CODEC_ID_UTVIDEO";
		else
		if(id == AV_CODEC_ID_BMV_VIDEO)
		   return (char*)"AV_CODEC_ID_BMV_VIDEO";
		else
		if(id == AV_CODEC_ID_VBLE)
		   return (char*)"AV_CODEC_ID_VBLE";
		else
		if(id == AV_CODEC_ID_DXTORY)
		   return (char*)"AV_CODEC_ID_DXTORY";
		else
		if(id == AV_CODEC_ID_V410)
		   return (char*)"AV_CODEC_ID_V410";
		else
		if(id == AV_CODEC_ID_XWD)
		   return (char*)"AV_CODEC_ID_XWD";
		else
		if(id == AV_CODEC_ID_CDXL)
		   return (char*)"AV_CODEC_ID_CDXL";
		else
		if(id == AV_CODEC_ID_XBM)
		   return (char*)"AV_CODEC_ID_XBM";
		else
		if(id == AV_CODEC_ID_ZEROCODEC)
		   return (char*)"AV_CODEC_ID_ZEROCODEC";
		else
		if(id == AV_CODEC_ID_MSS1)
		   return (char*)"AV_CODEC_ID_MSS1";
		else
		if(id == AV_CODEC_ID_MSA1)
		   return (char*)"AV_CODEC_ID_MSA1";
		else
		if(id == AV_CODEC_ID_TSCC2)
		   return (char*)"AV_CODEC_ID_TSCC2";
		else
		if(id == AV_CODEC_ID_MTS2)
		   return (char*)"AV_CODEC_ID_MTS2";
		else
		if(id == AV_CODEC_ID_CLLC)
		   return (char*)"AV_CODEC_ID_CLLC";
		else
		if(id == AV_CODEC_ID_MSS2)
		   return (char*)"AV_CODEC_ID_MSS2";
		else
		if(id == AV_CODEC_ID_VP9)
		   return (char*)"AV_CODEC_ID_VP9";
		else
		if(id == AV_CODEC_ID_AIC)
		   return (char*)"AV_CODEC_ID_AIC";
		else
		if(id == AV_CODEC_ID_BRENDER_PIX)
		   return (char*)"AV_CODEC_ID_BRENDER_PIX";
		else
		if(id == AV_CODEC_ID_Y41P)
		   return (char*)"AV_CODEC_ID_Y41P";
		else
		if(id == AV_CODEC_ID_ESCAPE130)
		   return (char*)"AV_CODEC_ID_ESCAPE130";
		else
		if(id == AV_CODEC_ID_EXR)
		   return (char*)"AV_CODEC_ID_EXR";
		else
		if(id == AV_CODEC_ID_AVRP)
		   return (char*)"AV_CODEC_ID_AVRP";
		else
		if(id == AV_CODEC_ID_012V)
		   return (char*)"AV_CODEC_ID_012V";
		else
		if(id == AV_CODEC_ID_G2M)
		   return (char*)"AV_CODEC_ID_G2M";
		else
		if(id == AV_CODEC_ID_AVUI)
		   return (char*)"AV_CODEC_ID_AVUI";
		else
		if(id == AV_CODEC_ID_AYUV)
		   return (char*)"AV_CODEC_ID_AYUV";
		else
		if(id == AV_CODEC_ID_TARGA_Y216)
		   return (char*)"AV_CODEC_ID_TARGA_Y216";
		else
		if(id == AV_CODEC_ID_V308)
		   return (char*)"AV_CODEC_ID_V308";
		else
		if(id == AV_CODEC_ID_V408)
		   return (char*)"AV_CODEC_ID_V408";
		else
		if(id == AV_CODEC_ID_YUV4)
		   return (char*)"AV_CODEC_ID_YUV4";
		else
		if(id == AV_CODEC_ID_SANM)
		   return (char*)"AV_CODEC_ID_SANM";
		else
		if(id == AV_CODEC_ID_PAF_VIDEO)
		   return (char*)"AV_CODEC_ID_PAF_VIDEO";
		else
		if(id == AV_CODEC_ID_AVRN)
		   return (char*)"AV_CODEC_ID_AVRN";
		else
		if(id == AV_CODEC_ID_CPIA)
		   return (char*)"AV_CODEC_ID_CPIA";
		else
		if(id == AV_CODEC_ID_XFACE)
		   return (char*)"AV_CODEC_ID_XFACE";
		else
		if(id == AV_CODEC_ID_SGIRLE)
		   return (char*)"AV_CODEC_ID_SGIRLE";
		else
		if(id == AV_CODEC_ID_MVC1)
		   return (char*)"AV_CODEC_ID_MVC1";
		else
		if(id == AV_CODEC_ID_MVC2)
		   return (char*)"AV_CODEC_ID_MVC2";
		else
		if(id == AV_CODEC_ID_SNOW)
		   return (char*)"AV_CODEC_ID_SNOW";
		else
		if(id == AV_CODEC_ID_WEBP)
		   return (char*)"AV_CODEC_ID_WEBP";
		else
		if(id == AV_CODEC_ID_SMVJPEG)
		   return (char*)"AV_CODEC_ID_SMVJPEG";
		else
		if(id == AV_CODEC_ID_HEVC)
		   return (char*)"AV_CODEC_ID_HEVC";
		else
		if(id == AV_CODEC_ID_FIRST_AUDIO)
		   return (char*)"AV_CODEC_ID_FIRST_AUDIO";
		else
		if(id == AV_CODEC_ID_PCM_S16LE)
		   return (char*)"AV_CODEC_ID_PCM_S16LE";
		else
		if(id == AV_CODEC_ID_PCM_S16BE)
		   return (char*)"AV_CODEC_ID_PCM_S16BE";
		else
		if(id == AV_CODEC_ID_PCM_U16LE)
		   return (char*)"AV_CODEC_ID_PCM_U16LE";
		else
		if(id == AV_CODEC_ID_PCM_U16BE)
		   return (char*)"AV_CODEC_ID_PCM_U16BE";
		else
		if(id == AV_CODEC_ID_PCM_S8)
		   return (char*)"AV_CODEC_ID_PCM_S8";
		else
		if(id == AV_CODEC_ID_PCM_U8)
		   return (char*)"AV_CODEC_ID_PCM_U8";
		else
		if(id == AV_CODEC_ID_PCM_MULAW)
		   return (char*)"AV_CODEC_ID_PCM_MULAW";
		else
		if(id == AV_CODEC_ID_PCM_ALAW)
		   return (char*)"AV_CODEC_ID_PCM_ALAW";
		else
		if(id == AV_CODEC_ID_PCM_S32LE)
		   return (char*)"AV_CODEC_ID_PCM_S32LE";
		else
		if(id == AV_CODEC_ID_PCM_S32BE)
		   return (char*)"AV_CODEC_ID_PCM_S32BE";
		else
		if(id == AV_CODEC_ID_PCM_U32LE)
		   return (char*)"AV_CODEC_ID_PCM_U32LE";
		else
		if(id == AV_CODEC_ID_PCM_U32BE)
		   return (char*)"AV_CODEC_ID_PCM_U32BE";
		else
		if(id == AV_CODEC_ID_PCM_S24LE)
		   return (char*)"AV_CODEC_ID_PCM_S24LE";
		else
		if(id == AV_CODEC_ID_PCM_S24BE)
		   return (char*)"AV_CODEC_ID_PCM_S24BE";
		else
		if(id == AV_CODEC_ID_PCM_U24LE)
		   return (char*)"AV_CODEC_ID_PCM_U24LE";
		else
		if(id == AV_CODEC_ID_PCM_U24BE)
		   return (char*)"AV_CODEC_ID_PCM_U24BE";
		else
		if(id == AV_CODEC_ID_PCM_S24DAUD)
		   return (char*)"AV_CODEC_ID_PCM_S24DAUD";
		else
		if(id == AV_CODEC_ID_PCM_ZORK)
		   return (char*)"AV_CODEC_ID_PCM_ZORK";
		else
		if(id == AV_CODEC_ID_PCM_S16LE_PLANAR)
		   return (char*)"AV_CODEC_ID_PCM_S16LE_PLANAR";
		else
		if(id == AV_CODEC_ID_PCM_DVD)
		   return (char*)"AV_CODEC_ID_PCM_DVD";
		else
		if(id == AV_CODEC_ID_PCM_F32BE)
		   return (char*)"AV_CODEC_ID_PCM_F32BE";
		else
		if(id == AV_CODEC_ID_PCM_F32LE)
		   return (char*)"AV_CODEC_ID_PCM_F32LE";
		else
		if(id == AV_CODEC_ID_PCM_F64BE)
		   return (char*)"AV_CODEC_ID_PCM_F64BE";
		else
		if(id == AV_CODEC_ID_PCM_F64LE)
		   return (char*)"AV_CODEC_ID_PCM_F64LE";
		else
		if(id == AV_CODEC_ID_PCM_BLURAY)
		   return (char*)"AV_CODEC_ID_PCM_BLURAY";
		else
		if(id == AV_CODEC_ID_PCM_LXF)
		   return (char*)"AV_CODEC_ID_PCM_LXF";
		else
		if(id == AV_CODEC_ID_S302M)
		   return (char*)"AV_CODEC_ID_S302M";
		else
		if(id == AV_CODEC_ID_PCM_S8_PLANAR)
		   return (char*)"AV_CODEC_ID_PCM_S8_PLANAR";
		else
		if(id == AV_CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED)
		   return (char*)"AV_CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED";
		else
		if(id == AV_CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED)
		   return (char*)"AV_CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED";
		else
		if(id == AV_CODEC_ID_PCM_S24LE_PLANAR)
		   return (char*)"AV_CODEC_ID_PCM_S24LE_PLANAR";
		else
		if(id == AV_CODEC_ID_PCM_S32LE_PLANAR)
		   return (char*)"AV_CODEC_ID_PCM_S32LE_PLANAR";
		else
		if(id == AV_CODEC_ID_PCM_S16BE_PLANAR)
		   return (char*)"AV_CODEC_ID_PCM_S16BE_PLANAR";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_QT)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_QT";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_WAV)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_WAV";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_DK3)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_DK3";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_DK4)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_DK4";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_WS)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_WS";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_SMJPEG)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_SMJPEG";
		else
		if(id == AV_CODEC_ID_ADPCM_MS)
		   return (char*)"AV_CODEC_ID_ADPCM_MS";
		else
		if(id == AV_CODEC_ID_ADPCM_4XM)
		   return (char*)"AV_CODEC_ID_ADPCM_4XM";
		else
		if(id == AV_CODEC_ID_ADPCM_XA)
		   return (char*)"AV_CODEC_ID_ADPCM_XA";
		else
		if(id == AV_CODEC_ID_ADPCM_ADX)
		   return (char*)"AV_CODEC_ID_ADPCM_ADX";
		else
		if(id == AV_CODEC_ID_ADPCM_EA)
		   return (char*)"AV_CODEC_ID_ADPCM_EA";
		else
		if(id == AV_CODEC_ID_ADPCM_G726)
		   return (char*)"AV_CODEC_ID_ADPCM_G726";
		else
		if(id == AV_CODEC_ID_ADPCM_CT)
		   return (char*)"AV_CODEC_ID_ADPCM_CT";
		else
		if(id == AV_CODEC_ID_ADPCM_SWF)
		   return (char*)"AV_CODEC_ID_ADPCM_SWF";
		else
		if(id == AV_CODEC_ID_ADPCM_YAMAHA)
		   return (char*)"AV_CODEC_ID_ADPCM_YAMAHA";
		else
		if(id == AV_CODEC_ID_ADPCM_SBPRO_4)
		   return (char*)"AV_CODEC_ID_ADPCM_SBPRO_4";
		else
		if(id == AV_CODEC_ID_ADPCM_SBPRO_3)
		   return (char*)"AV_CODEC_ID_ADPCM_SBPRO_3";
		else
		if(id == AV_CODEC_ID_ADPCM_SBPRO_2)
		   return (char*)"AV_CODEC_ID_ADPCM_SBPRO_2";
		else
		if(id == AV_CODEC_ID_ADPCM_THP)
		   return (char*)"AV_CODEC_ID_ADPCM_THP";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_AMV)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_AMV";
		else
		if(id == AV_CODEC_ID_ADPCM_EA_R1)
		   return (char*)"AV_CODEC_ID_ADPCM_EA_R1";
		else
		if(id == AV_CODEC_ID_ADPCM_EA_R3)
		   return (char*)"AV_CODEC_ID_ADPCM_EA_R3";
		else
		if(id == AV_CODEC_ID_ADPCM_EA_R2)
		   return (char*)"AV_CODEC_ID_ADPCM_EA_R2";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_EA_SEAD)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_EA_SEAD";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_EA_EACS)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_EA_EACS";
		else
		if(id == AV_CODEC_ID_ADPCM_EA_XAS)
		   return (char*)"AV_CODEC_ID_ADPCM_EA_XAS";
		else
		if(id == AV_CODEC_ID_ADPCM_EA_MAXIS_XA)
		   return (char*)"AV_CODEC_ID_ADPCM_EA_MAXIS_XA";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_ISS)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_ISS";
		else
		if(id == AV_CODEC_ID_ADPCM_G722)
		   return (char*)"AV_CODEC_ID_ADPCM_G722";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_APC)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_APC";
		else
		if(id == AV_CODEC_ID_VIMA)
		   return (char*)"AV_CODEC_ID_VIMA";
		else
		if(id == AV_CODEC_ID_ADPCM_AFC)
		   return (char*)"AV_CODEC_ID_ADPCM_AFC";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_OKI)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_OKI";
		else
		if(id == AV_CODEC_ID_ADPCM_DTK)
		   return (char*)"AV_CODEC_ID_ADPCM_DTK";
		else
		if(id == AV_CODEC_ID_ADPCM_IMA_RAD)
		   return (char*)"AV_CODEC_ID_ADPCM_IMA_RAD";
		else
		if(id == AV_CODEC_ID_ADPCM_G726LE)
		   return (char*)"AV_CODEC_ID_ADPCM_G726LE";
		else
		if(id == AV_CODEC_ID_AMR_NB)
		   return (char*)"AV_CODEC_ID_AMR_NB";
		else
		if(id == AV_CODEC_ID_AMR_WB)
		   return (char*)"AV_CODEC_ID_AMR_WB";
		else
		if(id == AV_CODEC_ID_RA_144)
		   return (char*)"AV_CODEC_ID_RA_144";
		else
		if(id == AV_CODEC_ID_RA_288)
		   return (char*)"AV_CODEC_ID_RA_288";
		else
		if(id == AV_CODEC_ID_ROQ_DPCM)
		   return (char*)"AV_CODEC_ID_ROQ_DPCM";
		else
		if(id == AV_CODEC_ID_INTERPLAY_DPCM)
		   return (char*)"AV_CODEC_ID_INTERPLAY_DPCM";
		else
		if(id == AV_CODEC_ID_XAN_DPCM)
		   return (char*)"AV_CODEC_ID_XAN_DPCM";
		else
		if(id == AV_CODEC_ID_SOL_DPCM)
		   return (char*)"AV_CODEC_ID_SOL_DPCM";
		else
		if(id == AV_CODEC_ID_MP2)
		   return (char*)"AV_CODEC_ID_MP2";
		else
		if(id == AV_CODEC_ID_MP3)
		   return (char*)"AV_CODEC_ID_MP3";
		else
		if(id == AV_CODEC_ID_AAC)
		   return (char*)"AV_CODEC_ID_AAC";
		else
		if(id == AV_CODEC_ID_AC3)
		   return (char*)"AV_CODEC_ID_AC3";
		else
		if(id == AV_CODEC_ID_DTS)
		   return (char*)"AV_CODEC_ID_DTS";
		else
		if(id == AV_CODEC_ID_VORBIS)
		   return (char*)"AV_CODEC_ID_VORBIS";
		else
		if(id == AV_CODEC_ID_DVAUDIO)
		   return (char*)"AV_CODEC_ID_DVAUDIO";
		else
		if(id == AV_CODEC_ID_WMAV1)
		   return (char*)"AV_CODEC_ID_WMAV1";
		else
		if(id == AV_CODEC_ID_WMAV2)
		   return (char*)"AV_CODEC_ID_WMAV2";
		else
		if(id == AV_CODEC_ID_MACE3)
		   return (char*)"AV_CODEC_ID_MACE3";
		else
		if(id == AV_CODEC_ID_MACE6)
		   return (char*)"AV_CODEC_ID_MACE6";
		else
		if(id == AV_CODEC_ID_VMDAUDIO)
		   return (char*)"AV_CODEC_ID_VMDAUDIO";
		else
		if(id == AV_CODEC_ID_FLAC)
		   return (char*)"AV_CODEC_ID_FLAC";
		else
		if(id == AV_CODEC_ID_MP3ADU)
		   return (char*)"AV_CODEC_ID_MP3ADU";
		else
		if(id == AV_CODEC_ID_MP3ON4)
		   return (char*)"AV_CODEC_ID_MP3ON4";
		else
		if(id == AV_CODEC_ID_SHORTEN)
		   return (char*)"AV_CODEC_ID_SHORTEN";
		else
		if(id == AV_CODEC_ID_ALAC)
		   return (char*)"AV_CODEC_ID_ALAC";
		else
		if(id == AV_CODEC_ID_WESTWOOD_SND1)
		   return (char*)"AV_CODEC_ID_WESTWOOD_SND1";
		else
		if(id == AV_CODEC_ID_GSM)
		   return (char*)"AV_CODEC_ID_GSM";
		else
		if(id == AV_CODEC_ID_QDM2)
		   return (char*)"AV_CODEC_ID_QDM2";
		else
		if(id == AV_CODEC_ID_COOK)
		   return (char*)"AV_CODEC_ID_COOK";
		else
		if(id == AV_CODEC_ID_TRUESPEECH)
		   return (char*)"AV_CODEC_ID_TRUESPEECH";
		else
		if(id == AV_CODEC_ID_TTA)
		   return (char*)"AV_CODEC_ID_TTA";
		else
		if(id == AV_CODEC_ID_SMACKAUDIO)
		   return (char*)"AV_CODEC_ID_SMACKAUDIO";
		else
		if(id == AV_CODEC_ID_QCELP)
		   return (char*)"AV_CODEC_ID_QCELP";
		else
		if(id == AV_CODEC_ID_WAVPACK)
		   return (char*)"AV_CODEC_ID_WAVPACK";
		else
		if(id == AV_CODEC_ID_DSICINAUDIO)
		   return (char*)"AV_CODEC_ID_DSICINAUDIO";
		else
		if(id == AV_CODEC_ID_IMC)
		   return (char*)"AV_CODEC_ID_IMC";
		else
		if(id == AV_CODEC_ID_MUSEPACK7)
		   return (char*)"AV_CODEC_ID_MUSEPACK7";
		else
		if(id == AV_CODEC_ID_MLP)
		   return (char*)"AV_CODEC_ID_MLP";
		else
		if(id == AV_CODEC_ID_GSM_MS)
		   return (char*)"AV_CODEC_ID_GSM_MS";
		else
		if(id == AV_CODEC_ID_ATRAC3)
		   return (char*)"AV_CODEC_ID_ATRAC3";
		else
		if(id == AV_CODEC_ID_APE)
		   return (char*)"AV_CODEC_ID_APE";
		else
		if(id == AV_CODEC_ID_NELLYMOSER)
		   return (char*)"AV_CODEC_ID_NELLYMOSER";
		else
		if(id == AV_CODEC_ID_MUSEPACK8)
		   return (char*)"AV_CODEC_ID_MUSEPACK8";
		else
		if(id == AV_CODEC_ID_SPEEX)
		   return (char*)"AV_CODEC_ID_SPEEX";
		else
		if(id == AV_CODEC_ID_WMAVOICE)
		   return (char*)"AV_CODEC_ID_WMAVOICE";
		else
		if(id == AV_CODEC_ID_WMAPRO)
		   return (char*)"AV_CODEC_ID_WMAPRO";
		else
		if(id == AV_CODEC_ID_WMALOSSLESS)
		   return (char*)"AV_CODEC_ID_WMALOSSLESS";
		else
		if(id == AV_CODEC_ID_ATRAC3P)
		   return (char*)"AV_CODEC_ID_ATRAC3P";
		else
		if(id == AV_CODEC_ID_EAC3)
		   return (char*)"AV_CODEC_ID_EAC3";
		else
		if(id == AV_CODEC_ID_SIPR)
		   return (char*)"AV_CODEC_ID_SIPR";
		else
		if(id == AV_CODEC_ID_MP1)
		   return (char*)"AV_CODEC_ID_MP1";
		else
		if(id == AV_CODEC_ID_TWINVQ)
		   return (char*)"AV_CODEC_ID_TWINVQ";
		else
		if(id == AV_CODEC_ID_TRUEHD)
		   return (char*)"AV_CODEC_ID_TRUEHD";
		else
		if(id == AV_CODEC_ID_MP4ALS)
		   return (char*)"AV_CODEC_ID_MP4ALS";
		else
		if(id == AV_CODEC_ID_ATRAC1)
		   return (char*)"AV_CODEC_ID_ATRAC1";
		else
		if(id == AV_CODEC_ID_BINKAUDIO_RDFT)
		   return (char*)"AV_CODEC_ID_BINKAUDIO_RDFT";
		else
		if(id == AV_CODEC_ID_BINKAUDIO_DCT)
		   return (char*)"AV_CODEC_ID_BINKAUDIO_DCT";
		else
		if(id == AV_CODEC_ID_AAC_LATM)
		   return (char*)"AV_CODEC_ID_AAC_LATM";
		else
		if(id == AV_CODEC_ID_QDMC)
		   return (char*)"AV_CODEC_ID_QDMC";
		else
		if(id == AV_CODEC_ID_CELT)
		   return (char*)"AV_CODEC_ID_CELT";
		else
		if(id == AV_CODEC_ID_G723_1)
		   return (char*)"AV_CODEC_ID_G723_1";
		else
		if(id == AV_CODEC_ID_G729)
		   return (char*)"AV_CODEC_ID_G729";
		else
		if(id == AV_CODEC_ID_8SVX_EXP)
		   return (char*)"AV_CODEC_ID_8SVX_EXP";
		else
		if(id == AV_CODEC_ID_8SVX_FIB)
		   return (char*)"AV_CODEC_ID_8SVX_FIB";
		else
		if(id == AV_CODEC_ID_BMV_AUDIO)
		   return (char*)"AV_CODEC_ID_BMV_AUDIO";
		else
		if(id == AV_CODEC_ID_RALF)
		   return (char*)"AV_CODEC_ID_RALF";
		else
		if(id == AV_CODEC_ID_IAC)
		   return (char*)"AV_CODEC_ID_IAC";
		else
		if(id == AV_CODEC_ID_ILBC)
		   return (char*)"AV_CODEC_ID_ILBC";
		else
		if(id == AV_CODEC_ID_OPUS_DEPRECATED)
		   return (char*)"AV_CODEC_ID_OPUS_DEPRECATED";
		else
		if(id == AV_CODEC_ID_COMFORT_NOISE)
		   return (char*)"AV_CODEC_ID_COMFORT_NOISE";
		else
		if(id == AV_CODEC_ID_TAK_DEPRECATED)
		   return (char*)"AV_CODEC_ID_TAK_DEPRECATED";
		else
		if(id == AV_CODEC_ID_METASOUND)
		   return (char*)"AV_CODEC_ID_METASOUND";
		else
		if(id == AV_CODEC_ID_FFWAVESYNTH)
		   return (char*)"AV_CODEC_ID_FFWAVESYNTH";
		else
		if(id == AV_CODEC_ID_SONIC)
		   return (char*)"AV_CODEC_ID_SONIC";
		else
		if(id == AV_CODEC_ID_SONIC_LS)
		   return (char*)"AV_CODEC_ID_SONIC_LS";
		else
		if(id == AV_CODEC_ID_PAF_AUDIO)
		   return (char*)"AV_CODEC_ID_PAF_AUDIO";
		else
		if(id == AV_CODEC_ID_OPUS)
		   return (char*)"AV_CODEC_ID_OPUS";
		else
		if(id == AV_CODEC_ID_TAK)
		   return (char*)"AV_CODEC_ID_TAK";
		else
		if(id == AV_CODEC_ID_EVRC)
		   return (char*)"AV_CODEC_ID_EVRC";
		else
		if(id == AV_CODEC_ID_SMV)
		   return (char*)"AV_CODEC_ID_SMV";
		else
		if(id == AV_CODEC_ID_FIRST_SUBTITLE)
		   return (char*)"AV_CODEC_ID_FIRST_SUBTITLE";
		else
		if(id == AV_CODEC_ID_DVD_SUBTITLE)
		   return (char*)"AV_CODEC_ID_DVD_SUBTITLE";
		else
		if(id == AV_CODEC_ID_DVB_SUBTITLE)
		   return (char*)"AV_CODEC_ID_DVB_SUBTITLE";
		else
		if(id == AV_CODEC_ID_TEXT)
		   return (char*)"AV_CODEC_ID_TEXT";
		else
		if(id == AV_CODEC_ID_XSUB)
		   return (char*)"AV_CODEC_ID_XSUB";
		else
		if(id == AV_CODEC_ID_SSA)
		   return (char*)"AV_CODEC_ID_SSA";
		else
		if(id == AV_CODEC_ID_MOV_TEXT)
		   return (char*)"AV_CODEC_ID_MOV_TEXT";
		else
		if(id == AV_CODEC_ID_HDMV_PGS_SUBTITLE)
		   return (char*)"AV_CODEC_ID_HDMV_PGS_SUBTITLE";
		else
		if(id == AV_CODEC_ID_DVB_TELETEXT)
		   return (char*)"AV_CODEC_ID_DVB_TELETEXT";
		else
		if(id == AV_CODEC_ID_SRT)
		   return (char*)"AV_CODEC_ID_SRT";
		else
		if(id == AV_CODEC_ID_MICRODVD)
		   return (char*)"AV_CODEC_ID_MICRODVD";
		else
		if(id == AV_CODEC_ID_EIA_608)
		   return (char*)"AV_CODEC_ID_EIA_608";
		else
		if(id == AV_CODEC_ID_JACOSUB)
		   return (char*)"AV_CODEC_ID_JACOSUB";
		else
		if(id == AV_CODEC_ID_SAMI)
		   return (char*)"AV_CODEC_ID_SAMI";
		else
		if(id == AV_CODEC_ID_REALTEXT)
		   return (char*)"AV_CODEC_ID_REALTEXT";
		else
		if(id == AV_CODEC_ID_SUBVIEWER1)
		   return (char*)"AV_CODEC_ID_SUBVIEWER1";
		else
		if(id == AV_CODEC_ID_SUBVIEWER)
		   return (char*)"AV_CODEC_ID_SUBVIEWER";
		else
		if(id == AV_CODEC_ID_SUBRIP)
		   return (char*)"AV_CODEC_ID_SUBRIP";
		else
		if(id == AV_CODEC_ID_WEBVTT)
		   return (char*)"AV_CODEC_ID_WEBVTT";
		else
		if(id == AV_CODEC_ID_MPL2)
		   return (char*)"AV_CODEC_ID_MPL2";
		else
		if(id == AV_CODEC_ID_VPLAYER)
		   return (char*)"AV_CODEC_ID_VPLAYER";
		else
		if(id == AV_CODEC_ID_PJS)
		   return (char*)"AV_CODEC_ID_PJS";
		else
		if(id == AV_CODEC_ID_ASS)
		   return (char*)"AV_CODEC_ID_ASS";
		else
		if(id == AV_CODEC_ID_FIRST_UNKNOWN)
		   return (char*)"AV_CODEC_ID_FIRST_UNKNOWN";
		else
		if(id == AV_CODEC_ID_TTF)
		   return (char*)"AV_CODEC_ID_TTF";
		else
		if(id == AV_CODEC_ID_BINTEXT)
		   return (char*)"AV_CODEC_ID_BINTEXT";
		else
		if(id == AV_CODEC_ID_XBIN)
		   return (char*)"AV_CODEC_ID_XBIN";
		else
		if(id == AV_CODEC_ID_IDF)
		   return (char*)"AV_CODEC_ID_IDF";
		else
		if(id == AV_CODEC_ID_OTF)
		   return (char*)"AV_CODEC_ID_OTF";
		else
		if(id == AV_CODEC_ID_SMPTE_KLV)
		   return (char*)"AV_CODEC_ID_SMPTE_KLV";
		else
		if(id == AV_CODEC_ID_DVD_NAV)
		   return (char*)"AV_CODEC_ID_DVD_NAV";
		else
		if(id == AV_CODEC_ID_PROBE)
		   return (char*)"AV_CODEC_ID_PROBE";
		else
		if(id == AV_CODEC_ID_MPEG2TS)
		   return (char*)"AV_CODEC_ID_MPEG2TS";
		else
		if(id == AV_CODEC_ID_MPEG4SYSTEMS)
		   return (char*)"AV_CODEC_ID_MPEG4SYSTEMS";
		else
		if(id == AV_CODEC_ID_FFMETADATA)
		   return (char*)"AV_CODEC_ID_FFMETADATA";

		return (char*)"CODEC NOT FOUND";
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
	if (!path || !strlen(path))
		return NULL;

	VuoMovieDecoder *decoder = new VuoMovieDecoder();

	if (decoder->initWithFile(path) != 0)
	{
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

	*duration = ((VuoMovieDecoder*)movie)->container.duration;

	VuoRetain(movie);
	VuoRelease(movie);

	return true;
}
