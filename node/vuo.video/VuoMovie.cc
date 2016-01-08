/**
 * @file
 * VuoMovie implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */
#include "VuoMovie.h"
#include <OpenGL/CGLMacro.h>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoGlContext.h"
#include "VuoGlPool.h"
#include "VuoReal.h"
#include "VuoList_VuoReal.h"
#include <sys/time.h>

extern "C"
{
#include "module.h"
#include <dispatch/dispatch.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <avcodec.h>
#include <avformat.h>
#include <avutil.h>
#include <swscale.h>
#include <string.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#pragma clang diagnostic pop

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMovie",
					 "dependencies" : [
						"VuoImage",
						"VuoAudioSamples",
						"VuoReal",
						"VuoList_VuoAudioSamples",
						"VuoList_VuoReal",
						"avcodec",
						"avformat",
						"avutil",
						"swscale",
						"swresample"
					 ],
					 "compatibleOperatingSystems": {
						 "macosx" : { "min": "10.6" }
					 }
				 });
#endif
}

#define SEC_PER_USEC .000001				///< Seconds per microsecond. Vuo nodes want information in seconds, where ffmpeg operates in microseconds.
#define REWIND .1							///< How many seconds behind the requested frame to seek.
#define REWIND_INCREMENT .2					///< If the REWIND value does not seek far enough, increment by this value.
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000	///< https://github.com/chelyaev/ffmpeg-tutorial/issues/13

#define AV_SYNC_THRESHOLD 0.01 				///< The maximum allowable latency in either direction for audio offset
#define AV_NOSYNC_THRESHOLD 10.0 			///< If the audio track is offset by greater than this amount, give up synching and reset.
#define SAMPLE_CORRECTION_PERCENT_MAX 24 	///< Maximum number of samples to add or subtract from a buffer when synching.
#define AUDIO_DIFF_AVG_NB 20 				///< Used in calculating audio offset.
#define MAX_PACKET_QUEUE_LENGTH 100			///< If we're not using a particular stream, keep the queue only up to this amount
/**
 * Instance class used to control the playback of video.
 * \sa @c VuoMovie_getNextFrame @c VuoMovie_initFfmpeg @c VuoMovie_getInfo @c VuoMovie_make
 */
class VuoMovieDecoder
{
public:

	/**
	 * A replacement for the FFMPEG AVPacketList which uses deprecated functionality.
	 */
	typedef struct VuoPacketList {
		AVPacket pkt;
		VuoPacketList *next;
	} VuoPacketList;

	/**
	 * Holds audio AVPackets for decoding.
	 */
	typedef struct PacketQueue
	{
		VuoPacketList *first_pkt, *last_pkt;
		int nb_packets;
		int size;
	} PacketQueue;

	/**
	 * Internal struct which contains context and current playback status of VuoMovieDecoder.
	 */
	typedef struct AVContainer
	{
		char *path;

		AVFormatContext *pFormatCtx;

		AVCodecContext *pCodecCtx;
		AVCodecContext *aCodecCtx;

		AVStream *video_st;
		AVStream *audio_st;

		PacketQueue audioPacketQueue;
		int bytesPerAudioSample;		// size of an audio sample (eg, sizeof(float), sizeof(double), etc)

		int64_t avOffset;	// first video packet PTS - first audio packet PTS

		PacketQueue videoPacketQueue;

		int videoStreamIndex;
		int audioStreamIndex;

		/* used for AV difference average computation */
		double audio_diff_cum;
		double audio_diff_avg_coef;
		double audio_diff_threshold;
		int audio_diff_avg_count;

		int64_t startPts;

		double duration;				// in seconds
		int packetDuration = 0;			// int time_base units
		int previousPacketDuration = 0;	// the duration of the packet immediately prior to the last queued packet.

		int64_t currentVideoTimestampUSEC = 0;		// in microseconds
		int64_t currentAudioTimestampUSEC = 0;		// in microseconds

		int64_t currentVideoPTS = 0;				// in av units
		int64_t currentAudioPTS = 0;				// in av units

		int64_t firstVideoFrame, lastVideoFrame;	// the first and last frame pts values
		int64_t firstAudioFrame, lastAudioFrame;	// first audio timestamp (PTS value)

		bool previousFrameValid;

		bool seekUnavailable;			// if true, the seek function will close and re-open it's stream
										// when seeking, then step to the desired frame.  necessary with
										// some codecs; namely flv and some gif files

		struct SwrContext *swr_ctx;
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
		packetQueueInit(&container.videoPacketQueue);

		nextVideoFrame(NULL);
		container.firstVideoFrame = container.currentVideoPTS;	// get the first frame's pts value, since format->start_pts isn't reliable

		if(containsAudio())
		{
			nextAudioFrame(NULL, 512);
			container.firstAudioFrame = container.currentAudioPTS;	// get the first frame's pts value, since format->start_pts isn't reliable
			container.avOffset = container.currentAudioTimestampUSEC - container.currentVideoTimestampUSEC;
		}

		// VLog("first audio timestamp : %f", container.currentAudioTimestampUSEC * SEC_PER_USEC);
		// VLog("first video timestamp : %f", container.currentVideoTimestampUSEC * SEC_PER_USEC)

		// VLog("==================");

		container.duration = av_rescale_q ( container.video_st->duration,  container.video_st->time_base, AV_TIME_BASE_Q ) * SEC_PER_USEC;

		// seek function clamps desired frame timestamp to first and last pts, so set it super high on the first run
		container.lastVideoFrame = INT64_MAX;
		container.lastAudioFrame = INT64_MAX;

		// seek to 1 microsecond before projected duration (if a duration value was available - otherwise we've got to step from start)
		// codecs like flash and some gif don't get seek support from ffmpeg, so do things the hard way.
		if(container.duration > 0. && !container.seekUnavailable)
			seekToMs( (container.duration * 1000)-1000  );

		while(nextVideoFrame(NULL)) ;

		container.lastVideoFrame = container.currentVideoPTS;

		if(containsAudio())
		{
			while(nextAudioFrame(NULL, 512)) ;
			container.lastAudioFrame = container.currentAudioPTS;
		}

		// VLog("File: %s\nCodec: %s", path, container.pCodecCtx->codec_name);
		// VLog("video: first pts: %lli last pts: %lli", container.firstVideoFrame, container.lastVideoFrame);
		// VLog("audio: first pts: %lli last pts: %lli", container.firstAudioFrame, container.lastAudioFrame);

		container.duration = av_rescale_q(container.lastVideoFrame-container.firstVideoFrame, container.video_st->time_base, AV_TIME_BASE_Q ) * SEC_PER_USEC;

		seekFrame(container.pCodecCtx, 0);

		// VLog("second duration is %f", container.duration);
		// int64_t sec = secondToAvTime(container.duration);
		// VLog("sec %f to pts is %lli", container.duration, sec);

		return 0;
	}

	/**
	 * If file contains audio tracks, return yes.  Otherwise, no.
	 */
	bool containsAudio()
	{
		return container.audioStreamIndex > -1;
	}

	/**
	 * Returns the last timestamp in seconds.
	 */
	double getCurrentSecond()
	{
		return container.currentVideoTimestampUSEC * SEC_PER_USEC;
	}

	/**
	 * Attempts to extract the frame image and timestamp for the next full frame in the current stream.
	 */
	bool getNextVideoFrame(VuoImage *image, double *frameTimestampInSeconds)
	{
		bool gotNextFrame = nextVideoFrame(image);

		if(gotNextFrame)
			*frameTimestampInSeconds = container.currentVideoTimestampUSEC * SEC_PER_USEC;

		return gotNextFrame;
	}

	/**
	 * Attempt to extract audioSamples.sampleCount number of values from audio channels
	 */
	bool getNextAudioFrame(VuoList_VuoAudioSamples audioSamples, double *frameTimestampInSeconds)
	{
		// nextAudioFrame will fill the channel array using 512 buckets at whatever sample rate ffmpeg specifies
		bool gotNextFrame = nextAudioFrame(audioSamples, VuoAudioSamples_bufferSize);

		if(gotNextFrame)
			*frameTimestampInSeconds = container.currentAudioTimestampUSEC * SEC_PER_USEC;

		return gotNextFrame;
	}

	/**
	 * Attempts to extract chronologically prior frame image and timestamp in the current stream.
	 */
	bool getPreviousVideoFrame(VuoImage *image, double *frameTimestampInSeconds)
	{
		// Check if we're at the beginning of the video, then attempt to seek to the frame prior to the one we want
		if( container.currentVideoPTS-container.previousPacketDuration <= container.firstVideoFrame ||
			!seekFrame(container.pCodecCtx, container.currentVideoPTS - container.previousPacketDuration) )
		{
			return false;
		}

		bool gotNextFrame = nextVideoFrame(image);

		if(gotNextFrame)
			*frameTimestampInSeconds = container.currentVideoTimestampUSEC * SEC_PER_USEC;

		return gotNextFrame;
	}

	/**
	 * Returns a @c VuoList_VuoReal containing every frame's presentation time stamp.  In the event that timestamp information is not available, the current frame index multiplied by time base is used.
	 */
	VuoList_VuoReal extractFramePtsValues()
	{
		VuoList_VuoReal framePts = VuoListCreate_VuoReal();

		//
		while(nextVideoFrame(NULL))
			VuoListAppendValue_VuoReal(framePts, container.currentVideoTimestampUSEC);

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

		return seekFrame(container.pCodecCtx, desiredFrameNumber);
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

		// hook up audio stream index
		container.audioStreamIndex = -1;
		for(int i = 0; i < (container.pFormatCtx)->nb_streams; i++)
			if( (container.pFormatCtx)->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				container.audioStreamIndex = i;
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

		// And the audio stream (if applicable)
		AVCodec *aCodec = NULL;
		if(container.audioStreamIndex > -1)
		{
			container.audio_st = (container.pFormatCtx)->streams[container.audioStreamIndex];
			container.aCodecCtx = container.audio_st->codec;
			aCodec = avcodec_find_decoder(container.aCodecCtx->codec_id);
		}

		if (pCodec == NULL)
		  return -1; // Codec not found

		// if(container.audioStreamIndex >= 0)
		// 	VLog("Audio Codec: %s", AVCodecIDToString(aCodec->id) );

		// Flash can't seek, so when seeking just jet to 0 and step
		if(pCodec->id == AV_CODEC_ID_FLV1 || pCodec->id == AV_CODEC_ID_GIF)
			container.seekUnavailable = true;

		// Open codec
		if(avcodec_open2(container.pCodecCtx, pCodec, NULL) < 0)
		  return -1; // Could not open codec

		if(container.audioStreamIndex >= 0)
		{
			int ret;
			if ((ret = avcodec_open2(container.aCodecCtx, aCodec, NULL)) < 0)
			{
				VLog("Unsupported audio codec %s: %s", AVCodecIDToString(container.aCodecCtx->codec_id), av_err2str(ret));
				container.audioStreamIndex = -1;
			}
			else
			{
				container.swr_ctx = swr_alloc();
				if (!container.swr_ctx)
				{
					VLog("Could not allocate resampler context\n");
					container.audioStreamIndex = -1;
				}
				else
				{
					/* set output resample options */
					int src_ch_layout = container.aCodecCtx->channel_layout;
					int src_rate = container.aCodecCtx->sample_rate;
					AVSampleFormat src_sample_fmt = container.aCodecCtx->sample_fmt;

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
					container.audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
					container.audio_diff_avg_count = 0;
					/* Correct audio only if larger error than this */

					// 512 is default sample size
					container.audio_diff_cum = .0;

					// Initialize audio_buffer that is used to store data straight from frames, and before
					// they're loaded into VuoAudioSamples arrays.
					audio_buf = (uint8_t **)calloc(container.aCodecCtx->channels, sizeof(uint8_t *));

					// use 1/30 because humans can't tell the difference less than this threshold
					container.audio_diff_threshold = 1./30;//2.0 * 512 / VuoAudioSamples_sampleRate;//container.aCodecCtx->sample_rate;

					int ret;
					if ((ret = swr_init(container.swr_ctx)) < 0)
						container.audioStreamIndex = -1;
					else
						/* initialize the packet queue */
						packetQueueInit(&container.audioPacketQueue);
				}
			}
		}

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
		SwsContext *img_convert_ctx = sws_getContext(width, height, pix_fmt, width, height, dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

		int result = sws_scale(img_convert_ctx, src->data, src->linesize, 0, height, dst->data, dst->linesize);
		sws_freeContext(img_convert_ctx);

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
	bool seekFrame(AVCodecContext *codexCtx, int64_t frame)
	{
		// container.previousFrameValid = false;

		packetQueueFlush(&container.videoPacketQueue);
		avcodec_flush_buffers(container.pCodecCtx);

		if(containsAudio())
		{
			packetQueueFlush(&container.audioPacketQueue);
			avcodec_flush_buffers(container.aCodecCtx);
		}

		if(frame > container.lastVideoFrame)
			frame = container.lastVideoFrame;

		if(frame < container.firstVideoFrame)
			frame = container.firstVideoFrame;

		int64_t seek = frame - secondToAvTime(rewind);

		int ret = av_seek_frame(container.pFormatCtx, container.videoStreamIndex, seek, AVSEEK_FLAG_ANY);

		container.currentVideoPTS = 0;

		if(ret < 0)
		{
			if(container.seekUnavailable)
			{
				if(codexCtx != NULL)
					avcodec_close(codexCtx);

				if(container.pFormatCtx != NULL)
					avformat_close_input(&container.pFormatCtx);

				if(avformat_open_input(&(container.pFormatCtx), container.path, NULL, NULL) != 0)
					return false;

				initCodecCtx();
			}
			else
				return false;
		}

		if(frame <= container.firstVideoFrame)
			return nextVideoFrame(NULL);

		int framesDecoded = 0;

		// after initial seek, step frame by frame til we get to right before the one we want.
		while(container.currentVideoPTS + container.packetDuration < frame)
		{
		// 	VLog("seeking: %llu + %i / %llu", container.currentVideoPTS, frameDuration, frame);

			// don't bail on a null frame in this loop, since nextFrame() can sometimes
			// decode a junk frame and return false when there are in fact frames still
			// left.  test with `/System/Library/Compositions/Yosemite.mov` to see this
			// behavior.

			int64_t previousFrame = container.currentVideoPTS;

			if( !nextVideoFrame(NULL) )
			{
					// VLog("reset frame: %llu -> %llu", frame, container.currentVideoPTS);
					frame = container.currentVideoPTS;
			}
			else
			{
				framesDecoded++;
			}

			// av_seek_frame can put the index before or after the desired frame.  when it overshoots,
			// seek to a further back point to guarantee we get a frame before and save the rewind
			// time used so that in future cycles this won't be necessary.
			if(container.currentVideoPTS >= frame)
			{
				// VLog("rewind: %llu > %llu", container.currentVideoPTS, frame);

				// if a frame has been stepped and we still went over, that means
				// packetDuration is not correct.  step to the frame prior to this'n.
				if(framesDecoded > 1)
				{
					frame = previousFrame;
					// VLog("set target frame: %llu", frame);
				}

				framesDecoded = 0;

				if(codexCtx != NULL)
				{
					packetQueueFlush(&container.videoPacketQueue);
					avcodec_flush_buffers(container.pCodecCtx);

					if(containsAudio())
					{
						packetQueueFlush(&container.audioPacketQueue);
						avcodec_flush_buffers(container.aCodecCtx);
					}
				}

				rewind += rewindIncrement;
				ret = av_seek_frame(container.pFormatCtx, container.videoStreamIndex, frame-secondToAvTime(rewind), AVSEEK_FLAG_ANY);
				if(ret < 0) return false;

				container.currentVideoPTS = container.firstVideoFrame;
			}
		}

		// update
		container.currentVideoTimestampUSEC = av_rescale_q ( container.currentVideoPTS,  container.video_st->time_base, AV_TIME_BASE_Q );

		/// also step the audio to match
		if(containsAudio())
		{
			do {
				nextAudioFrame(NULL, 8);
			} while( container.currentAudioTimestampUSEC - container.avOffset < container.currentVideoTimestampUSEC );
		}

		// `<=` because `0 == 0`, and in the event that we actually are a single frame too far ahead, well, that's okay too. 
		return container.currentVideoPTS <= frame;
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
		image = VuoImage_makeFromBuffer(pFrameRGB->data[0], GL_RGBA, pCodecCtx->width, pCodecCtx->height, VuoImageColorDepth_8);

		// Free the RGB image
		av_free(buffer);
		av_free(pFrameRGB);

		return image;
	}

	/**
	 *	Get next packet and put it in the audio or video queue.  Should
	 * 	only ever be called by next{Audio, Video}Frame() methods.
	 */
	bool nextFrame(PacketQueue *videoPktList, PacketQueue *audioPktList)
	{
		AVPacket packet;
		bool gotPacket = false;

		while(av_read_frame(container.pFormatCtx, &packet) >= 0)
		{
			// Is this a packet from the video stream?
			if(packet.stream_index == container.videoStreamIndex)
			{
				if(videoPktList != NULL)
					packetQueue_put(videoPktList, &packet);
				else
					av_free_packet(&packet);

				gotPacket = true;

				break;
			}
			else if(packet.stream_index == container.audioStreamIndex)
			{
				if(audioPktList != NULL)
					packetQueue_put(audioPktList, &packet);
				else
					av_free_packet(&packet);

				gotPacket = true;
				break;
			}
			else
			{
				av_free_packet(&packet);
			}
		}

		return gotPacket;
	}

	/**
	 * Decodes the next available video packet in videoPacketQueue and puts the image in *image
	 */
	bool nextVideoFrame(VuoImage *image)
	{
		// VLog("nextVideoFrame");
		AVFrame *pFrame = avcodec_alloc_frame();
		int frameFinished = 0;
		AVPacket packet;

		while( !frameFinished )
		{
			// try to get the next video packet from the queue, and if the queue is empty,
			// try to decode more packets.  if that fails, then return.
			while( packetQueue_get(&container.videoPacketQueue, &packet) < 0 )
			{
				// no more packets available
				if( !nextFrame(&container.videoPacketQueue, &container.audioPacketQueue) )
				{
					return false;
				}
			}

			avcodec_decode_video2(container.pCodecCtx, pFrame, &frameFinished, &packet);
		}

		// Did we get a video frame?
		if(frameFinished)
		{
			// Get PTS here because formats with I frames can return junk values before a full frame is found
			int64_t pts = av_frame_get_best_effort_timestamp ( pFrame );

			// Duration of this packet in AVStream->time_base units, 0 if unknown.
			container.previousPacketDuration = container.previousFrameValid ? pts - container.currentVideoPTS : container.packetDuration;	// packet.duration isn't guaranteed to be accurate, and often isn't.
			container.packetDuration = packet.duration == 0 ? container.previousPacketDuration : packet.duration;

			container.currentVideoPTS = pts;

			if( pts == AV_NOPTS_VALUE )
				pts = 1;//pts = container.frameCount * av_q2d(container.video_st->time_base);	// if no pts, guess using the time base and current frame count.
			else
				pts = av_rescale_q ( pts,  container.video_st->time_base, AV_TIME_BASE_Q );  // pts is now in microseconds.

			container.currentVideoTimestampUSEC = pts;


			if(image != NULL)
				*image = vuoImageWithAvFrame(container.pCodecCtx, pFrame);

			av_free_packet(&packet);
			av_free(pFrame);

			container.previousFrameValid = true;

			return true;
		}
		else
		{
			av_free(pFrame);

			return false;
		}
	}

	unsigned int audio_buf_size = 0;		// how many bytes the audio_decode_frame function filled audio_buf with
	unsigned int audio_buf_index = 0;	// the current index of read samples from audio_buf

	/* get all samples in the next packet and store them here */
	uint8_t **audio_buf = NULL;

	/**
	 * Read enough packets to fill the audioSamples.samples buffer, and store what's left for the next go-round

	 */
	bool nextAudioFrame(VuoList_VuoAudioSamples audioSamples, const int wantedSampleCount)
	{
		// VLog("nextAudioFrame");
		uint channel_count = container.aCodecCtx->channels;

		long len = wantedSampleCount * sizeof(double);

		int len1, audio_size = -1;

		unsigned int stream_index = 0;

		if(audioSamples == NULL)
		{
			if(audio_buf_size > 0)
			{
				for(int i = 0; i < channel_count; i++)
				{
					free(audio_buf[i]);
				}
			}

			audio_size = audio_decode_frame(audio_buf);

			if(audio_size < 0)
			{
				audio_buf_size = 0;
				return false;
			}
			else
			{
				audio_buf_size = audio_size;
				audio_buf_index = 0;
				return true;
			}
		}

		/* init as many audiosamples arrays as channels and alloc with wanted sample size */
		for(int i = 0; i < channel_count; i++)
		{
			VuoAudioSamples as = VuoAudioSamples_alloc(wantedSampleCount);

			as.samplesPerSecond = container.aCodecCtx->sample_rate;
			VuoListAppendValue_VuoAudioSamples(audioSamples, as);
		}

		while(stream_index < len)
		{
			if(audio_buf_index >= audio_buf_size)
			{
				/* We have already sent all our data; get more */
				if(audio_buf_size > 0)
				{
					for(int i = 0; i < channel_count; i++)
					{
						free(audio_buf[i]);
					}
				}

				audio_size = audio_decode_frame(audio_buf);

				if(audio_size < 0)
				{
					// VLog("... silence ...");

					/* If error, output silence */
					audio_buf_size = 512 * sizeof(double);
					for(int i = 0; i < channel_count; i++)
					{
						audio_buf[i] = (uint8_t*)calloc(sizeof(double)*wantedSampleCount, sizeof(double));
						audio_size = wantedSampleCount * sizeof(double);

						memset(audio_buf[i], 0., audio_buf_size);
					}
				}
				else
				{
					audio_size = synchronize_audio(audio_buf, audio_size);
					audio_buf_size = audio_size;
				}

				// VLog("Decode Audio -> samples: %li", audio_buf_size / sizeof(double));

				audio_buf_index = 0;
			}

			len1 = audio_buf_size - audio_buf_index;

			if(len1 + stream_index > len)
				len1 = len - stream_index;

			// copy to stream and update index and length tallies
			for(int i = 0; i < channel_count; i++)
			{
				VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(audioSamples, i+1);
				memcpy( as.samples + stream_index/sizeof(double), audio_buf[i] + audio_buf_index, len1);
			}

			stream_index += len1;
			audio_buf_index += len1;
		}

		return true;
	}

	AVPacket pkt;
	uint8_t *audio_pkt_data = NULL;
	int audio_pkt_size = 0;
	int audio_decode_frame(uint8_t **audio_buf)
	{
		AVFrame *frame = avcodec_alloc_frame();
		container.aCodecCtx->request_sample_fmt = AV_SAMPLE_FMT_FLTP;

		int len1, data_size = 0;
		int converted_sample_count = 0;
		uint channel_count = container.aCodecCtx->channels;

		for(;;)
		{
			while(audio_pkt_size > 0)
			{
				int got_frame = 0;

				len1 = avcodec_decode_audio4(container.aCodecCtx, frame, &got_frame, &pkt);

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
					int64_t pts = av_frame_get_best_effort_timestamp ( frame );

					container.currentAudioPTS = pts;

					if( pts != AV_NOPTS_VALUE )
					{
						// todo - what if no pts value?
						pts = av_rescale_q ( pts,  container.audio_st->time_base, AV_TIME_BASE_Q );  // pts is now in microseconds.
						container.currentAudioTimestampUSEC = pts;
					}

					// data_size = frame.linesize[0];	// LIES
					data_size = frame->nb_samples * container.bytesPerAudioSample;

					// convert frame data to double planar
					uint8_t **dst_data;
					int dst_linesize;

					// figure out how many samples should come out of swr_convert
					int dst_nb_samples = av_rescale_rnd(swr_get_delay(container.swr_ctx, container.aCodecCtx->sample_rate) +
						frame->nb_samples, VuoAudioSamples_sampleRate, container.aCodecCtx->sample_rate, AV_ROUND_UP);

					/* allocate and fill destination double* arrays */
					int ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, channel_count, dst_nb_samples, AV_SAMPLE_FMT_DBLP, 0);
					if(ret < 0) VLog("Failed allocating double** array");

					/**
					 *	ret returns the new number of samples per channel
					 */
					ret = swr_convert(container.swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
					if(ret < 0) VLog("Failed conversion!");

					converted_sample_count = ret;

					/*
					 * For planar sample formats, each audio channel is in a separate data plane, and linesize is the
					 * buffer size, in bytes, for a single plane. All data planes must be the same size. For packed
					 * sample formats, only the first data plane is used, and samples for each channel are interleaved.
					 * In this case, linesize is the buffer size, in bytes, for the 1 plane.
					 */
					for(int i = 0; i < channel_count; i++)
					{
						audio_buf[i] = (uint8_t *)calloc(sizeof(double) * converted_sample_count, sizeof(double));
						memcpy(audio_buf[i], dst_data[i], dst_linesize);
					}

					av_free(frame);

					if (dst_data)
						av_freep(&dst_data[0]);

					av_freep(&dst_data);
				}

				/* No data yet, get more frames */
				if(data_size <= 0)
					continue;
				else
				{
					/* We have data, return it and come back for more later */
					return converted_sample_count * sizeof(double);
				}
			}

			if(pkt.data)
				av_free_packet(&pkt);

			while(packetQueue_get(&container.audioPacketQueue, &pkt) < 0)
			{
				if(!nextFrame(&container.videoPacketQueue, &container.audioPacketQueue))
				{
					// VLog("out of audio packets to decode");
					return -1;
				}
			}

			audio_pkt_data = pkt.data;
			audio_pkt_size = pkt.size;
		}
	}

	/**
	 *	http://dranger.com/ffmpeg/tutorial06.html
	 */
	int synchronize_audio(uint8_t **samples, int samples_size)//, double pts)
	{
		double diff, avg_diff;
		int wanted_size, min_size, max_size;//, nb_samples;

		// diff is in seconds
		// diff = ((double)container.currentAudioTimestampUSEC - container.currentVideoTimestampUSEC) * SEC_PER_USEC;
		diff = (double) ((container.currentAudioTimestampUSEC- container.avOffset) - container.currentVideoTimestampUSEC) * SEC_PER_USEC;

		if(diff < AV_NOSYNC_THRESHOLD)
		{
			// accumulate the diffs
			container.audio_diff_cum = diff + container.audio_diff_avg_coef * container.audio_diff_cum;

			if(container.audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
			{
				container.audio_diff_avg_count++;
			}
			else
			{
				avg_diff = container.audio_diff_cum * (1.0 - container.audio_diff_avg_coef);

				if(fabs(avg_diff) >= container.audio_diff_threshold)
				{
					wanted_size = samples_size + ((int)(diff * VuoAudioSamples_sampleRate) * sizeof(double));
					// wanted_size = samples_size + ((int)(diff * container.audio_st->codec->sample_rate) * sizeof(double));

					min_size = samples_size * ((100. - SAMPLE_CORRECTION_PERCENT_MAX) / 100.);
					max_size = samples_size * ((100. + SAMPLE_CORRECTION_PERCENT_MAX) / 100.);

					if(wanted_size < min_size)
					{
						wanted_size = min_size;
					}
					else if (wanted_size > max_size)
					{
						wanted_size = max_size;
					}

					wanted_size = (wanted_size - (wanted_size % 8));

					if(wanted_size < samples_size)
					{
						/* remove samples */
						samples_size = wanted_size;

						// VLog("remove samples a: %f v: %f  offset: %f  diff: %f",
						// 	(double)container.currentAudioTimestampUSEC * SEC_PER_USEC,
						// 	(double)container.currentVideoTimestampUSEC * SEC_PER_USEC,
						// 	container.avOffset * SEC_PER_USEC,
						// 	diff);
					}
					else if(wanted_size > samples_size)
					{
						// VLog("add samples a: %f v: %f  offset: %f  diff: %f",
						// 	(double)container.currentAudioTimestampUSEC * SEC_PER_USEC,
						// 	(double)container.currentVideoTimestampUSEC * SEC_PER_USEC,
						// 	container.avOffset * SEC_PER_USEC,
						// 	diff);

						/* add samples by copying final sample */
						uint channels = container.aCodecCtx->channels;
						for(int i = 0; i < channels; i++)
						{
							uint8_t *cp = (uint8_t*)calloc(wanted_size, sizeof(double));

							memcpy(cp, samples[i], samples_size);
							memset(cp + samples_size, samples[i][(samples_size-1)/sizeof(double)], wanted_size - samples_size);
							free(samples[i]);
							samples[i] = cp;
						}

						samples_size = wanted_size;
					}
				}
			}
		}
		else
		{
			/* difference container TOO big; reset diff stuff */
			container.audio_diff_avg_count = 0;
			container.audio_diff_cum = 0;
		}

		return samples_size;
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

	/**
	 * Initialize the PacketQueue
	 */
	void packetQueueInit(PacketQueue *q)
	{
		memset(q, 0, sizeof(PacketQueue));
		q->nb_packets = 0;
		q->size = 0;
	}

	/**
	 * Clear all packets in the queue
	 */
	void packetQueueFlush(PacketQueue *q)
	{
		AVPacket pkt;

		while(packetQueue_get(q, &pkt) >= 0)
		{
			av_free_packet(&pkt);
		}

		q->nb_packets = 0;
		q->size = 0;
	}

	int packetQueue_put(PacketQueue *q, AVPacket *pkt)
	{
		VuoPacketList *pkt1;

		// allocate the packet
		if(av_dup_packet(pkt) < 0)
			return -1;

		pkt1 = (VuoPacketList*)av_malloc(sizeof(VuoPacketList));
		if (!pkt1)	return -1;

		pkt1->pkt = *pkt;
		pkt1->next = NULL;

		// packetqueue is empty, add this to start; else add this to the end of the train
		if (!q->last_pkt)
			q->first_pkt = pkt1;
		else
			q->last_pkt->next = pkt1;

		// set this packet as the last in the train
		q->last_pkt = pkt1;

		// update the packet amount count
		q->nb_packets++;

		// and update the total size tally
		q->size += pkt1->pkt.size;

		// if we're silencing audio, keep the packet queue tidy
		while(q->nb_packets > MAX_PACKET_QUEUE_LENGTH)
		{
			AVPacket pkt;
			packetQueue_get(q, &pkt);
			av_free_packet(&pkt);
		}

		return 0;
	}

	/**
	 * @param q The PacketQueue to extract a packet from
	 * @param pkt (out) - extracted packet (if method returns 0).
	 */
	static int packetQueue_get(PacketQueue *q, AVPacket *pkt)
	{
		VuoPacketList *pkt1 = NULL;
		int ret;

		// get the first packet in the queue
		pkt1 = q->first_pkt;

		if (pkt1)
		{
			q->first_pkt = pkt1->next;

			// if this was the last packet in the queue, set queue first/last to NULL
			if (!q->first_pkt)
				q->last_pkt = NULL;

			q->nb_packets--;
			q->size -= pkt1->pkt.size;

			// store extracted packet in `out pkt`
			*pkt = pkt1->pkt;

			av_free(pkt1);

			ret = 0;
		}
		else
		{
			ret = -1;
		}

		return ret;
	}

};

/**
 * Initializes the FFMPEG libraries.
 */
static void __attribute__((constructor)) VuoMovie_initFfmpeg(void)
{
	av_register_all();
	avformat_network_init();

	// av_log_set_level(AV_LOG_VERBOSE);
	av_log_set_level(AV_LOG_FATAL);
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
bool VuoMovie_getNextVideoFrame(VuoMovie movie, VuoImage *image, double *nextFrame)
{
	return ((VuoMovieDecoder*)movie)->getNextVideoFrame(image, nextFrame);
}

/**
 * Returns true if audio channels are present in file, false if not.
 */
bool VuoMovie_containsAudio(VuoMovie movie)
{
	return ((VuoMovieDecoder*)movie)->containsAudio();
}

/**
 * Return the next available chunk of audio samples.  If none are available, return false.
 */
bool VuoMovie_getNextAudioSample(VuoMovie movie, VuoList_VuoAudioSamples audioSamples, double *frameTimestampInSeconds)
{
	// UInt8 *stream = (UInt8*)malloc(sizeof(UInt8)*VuoAudioSamples_bufferSize);
	// ((VuoMovieDecoder*)movie)->audio_callback(stream, VuoAudioSamples_bufferSize);

	return ((VuoMovieDecoder*)movie)->getNextAudioFrame(audioSamples, frameTimestampInSeconds) >= 0;
}

/**
 * Returns the frame prior to the internal currently queued frame, and the presentation timestamp associated.
 */
bool VuoMovie_getPreviousVideoFrame(VuoMovie movie, VuoImage *image, double *previousFrame)
{
	return ((VuoMovieDecoder*)movie)->getPreviousVideoFrame(image, previousFrame);
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
