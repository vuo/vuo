/**
 * @file
 * VuoFfmpegDecoder interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#include "module.h"
#include "VuoVideoDecoder.h"
#include "VuoVideo.h"
#include "VuoImage.h"
#include "VuoAudioSamples.h"
#include "VuoAudioFrame.h"
#include "VuoList_VuoAudioSamples.h"
#include "VuoVideoFrame.h"
#include "VuoText.h"
#include "VuoUrl.h"
#endif

// FFMPEG
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

#include <sys/types.h> // for uint

/**
 * An object for controlling and extracting information from video using FFMPEG.
 */
class VuoFfmpegDecoder : public VuoVideoDecoder
{
public:
	/// Initialize a instance of the FFMPEG decoder.  Can return NULL if initialization fails (but will clean up after itself in that event).
	static VuoFfmpegDecoder* Create(VuoUrl url);

	/// Unload objects allocated by the FFMPEG decoder.
	virtual ~VuoFfmpegDecoder();
	/// Get the next video frame in the queue.  If playback speed is negative, this will the the frame prior to the last retrieved.
	virtual bool NextVideoFrame(VuoVideoFrame* frame);
	/// Get the next audio frame in the queue.  If playback speed is negative (or anything other than 1), this returns nothing.
	virtual bool NextAudioFrame(VuoAudioFrame* audio);
	/// Seek the playhead to the second.  `second` is not in timestamp format, rather, relative to movie start = 0.
	virtual bool SeekToSecond(double second);
	/// Returns true if video contains an audio track.
	bool ContainsAudio();
	/// The total duration of this video in seconds
	virtual double GetDuration();
	/// Set the playback rate.  Any value that isn't 1 will flush the audio queue and discard any future audio frames until
	/// frame rate is back to 1.
	virtual void SetPlaybackRate(double rate);
	/// Returns the timestamp of the last decoded video frame.
	virtual double GetLastDecodedVideoTimeStamp();
	/// Returns true if there isn't an audio track, or there is but the decoder can play it.
	virtual bool CanPlayAudio();
	/// Returns the number of audio channels available.
	virtual unsigned int GetAudioChannelCount();
	/// Returns true if this asset is loaded and ready to play.
	virtual bool IsReady() { return isReady; }
	/// Returns the average frame-rate.
	virtual double GetFrameRate();

private:

	/**
	 * Linked list node used to store video or audio data.
	 */
	template<class T> struct LLNode
	{
		T data;
		LLNode* next;
		LLNode* previous;

		LLNode(T value)
		{
			data = value;
			next = NULL;
			previous = NULL;
		}
	};

	/**
	 * Simple linked list implementation, used to store caches of video frame and audio data.
	 */
	template<class T> struct LinkedList
	{
		LLNode<T>* first;	// pointer to the first node in this list.
		LLNode<T>* last;	// pointer to the last node in the list

		void (*destructor)(T*);

		unsigned int _size;	// how many nodes are in this list
		unsigned int Count() { return _size; }

		LinkedList()
		{
			_size = 0;
			first = NULL;
			last = NULL;
			destructor = NULL;
		}

		~LinkedList()
		{
			Clear();
		}

		void Clear()
		{
			LLNode<T>* cur = first;

			while(cur)
			{
				if(destructor != NULL)
					destructor(&cur->data);
				LLNode<T>* tmp = cur;
				cur = tmp->next;
				delete(tmp);
			}

			_size = 0;

			first = NULL;
			last = NULL;
		}

		void Add(T value)
		{
			LLNode<T>* node = new LLNode<T>(value);

			if( last )
			{
				node->previous = last;
				last->next = node;
				last = node;
			}
			else
			{
				first = node;
				last = node;
			}

			_size++;
		}

		/**
		 * Puts a new node at the front of the list
		 */
		void Unshift(T value)
		{
			LLNode<T>* node = new LLNode<T>(value);

			if(first != NULL)
			{
				first->previous = node;
				node->next = first;
				first = node;
			}
			else
			{
				first = node;
				last = node;
			}

			_size++;
		}

		void Remove(LLNode<T>* node)
		{
			if(node == NULL)
				return;

			if(node->next)
			{
				if(node->previous)
				{
					node->previous->next = node->next;
					node->next->previous = node->previous;
				}
				else
				{
					// was first node
					node->next->previous = NULL;
					first = node->next;
				}
			}
			else
			{
				// was last node in list
				if(node->previous)
				{
					last = node->previous;
					node->previous->next = NULL;
				}
				// was only node in list
				else
				{
					first = NULL;
					last = NULL;
				}
			}

			_size--;
			delete(node);
		}

		/**
		 * Returns and removes the first item from this list.
		 */
		bool Shift(T* value)
		{
			if(first != NULL)
			{
				*value = first->data;
				LLNode<T>* tmp = first;

				if(tmp->next == NULL)
				{
					first = NULL;
					last = NULL;
				}
				else
				{
					first = first->next;
				}

				delete(tmp);
				_size--;

				return true;
			}
			else
			{
				return false;
			}
		}

		/**
		 * Return and remove the last item in this list.
		 */
		bool Pop(T* value)
		{
			if(last != NULL)
			{
				*value = last->data;

				LLNode<T>* tmp = last;

				if(last->previous == NULL)
				{
					first = NULL;
					last = NULL;
				}
				else
				{
					last = last->previous;
					last->next = NULL;
				}

				delete(tmp);
				_size--;

				return true;
			}

			return false;
		}

		/**
		 * Sets value to the first item in list, but does not remove it from list.
		 */
		bool Peek(T* value)
		{
			if(first != NULL)
			{
				*value = first->data;
				return true;
			}
			return false;
		}

		bool PeekLast(T* value)
		{
			if(last != NULL)
			{
				*value = last->data;
				return true;
			}
			return false;
		}
	};

	/**
	 * Holds a list of VuoAudioSample, and the presentation timestamp in seconds.
	 */
	struct AudioFrame
	{
		unsigned int size;
		unsigned int channels;
		int64_t pts;
		double timestamp;
		uint8_t** samples;

		static void Delete(AudioFrame* frame)
		{
			for(int i = 0; i < frame->channels; i++)
				free(frame->samples[i]);
			free(frame->samples);
		}
	};

	struct VideoFrame
	{
		VuoImage image;
		int64_t pts;
		double timestamp;
		double duration;

		static void Delete(VideoFrame* frame)
		{
			VuoRelease(frame->image);
		}
	};

	LinkedList<AudioFrame> audioFrames;
	LinkedList<VideoFrame> videoFrames;
	LinkedList<AVPacket> videoPackets;
	LinkedList<AVPacket> audioPackets;

	struct VideoInfo
	{
		int64_t first_pts;		// first pts value in video stream time
		int64_t last_pts;		// finish in video stream time
		int64_t duration;		// duration in video stream time
	};

	/**
	 * Internal struct which contains context and current playback status of VuoMovieDecoder.
	 */
	struct AVContainer
	{
		AVFormatContext* 	formatCtx;
		AVCodecContext* 	videoCodecCtx;
		AVCodecContext* 	audioCodecCtx;

		struct SwrContext *swr_ctx;	// for resampling audio
		int bytesPerAudioSample;	// Size of an audio samples (eg, sizeof(float), sizeof(int), etc).

		// Stream contains information about the channel, ex time base and length.
		int videoStreamIndex, audioStreamIndex;
		AVStream* videoStream;
		AVStream* audioStream;

		VideoInfo videoInfo;
		VideoInfo audioInfo;

		bool seekUnavailable;
	};

	AVContainer container;

	/// The path to the movie file.
	VuoUrl mVideoPath;

	/// How fast and in which direction the video is being decoded. Negative values are allowed.  A value of 1 means
	/// video is playing at the speed requested by the file, where 2 is twice as fast, and -1 is reversed.
	double mPlaybackRate;

	int64_t lastDecodedVideoPts;
	int64_t lastSentVideoPts;
	int64_t lastDecodedAudioPts;

	double lastVideoTimestamp;	// Last sent video timestamp in seconds, relative to 0 being start of movie (actual timestamp could be negative, or non-zero)
	double lastAudioTimestamp;	// Last sent audio timestamp in seconds.

	// Audio sync cumulative difference in last decoded timestamp.
	double audio_diff_cum;
	double audio_diff_avg_coef;
	// The threshold at which to begin modifying returned audio sample arrays to better sync with video.
	double audio_diff_threshold;
	int audio_diff_avg_count;

	// If the playback rate is 1, enable audio
	bool audioIsEnabled;

	// Holds decoded audio between extraction from frame and conversion to VuoAudioSamples.
	uint8_t** audio_buf;

	// The size of a single channel of audio in audio_buf, set by DecodeAudio().  ( sampleCount * sizeof(double) )
	unsigned int audio_buf_size;

	// the current index in audio_buff (per-channel) that is being copied to VuoAudioSamples
	unsigned int audio_buf_index;

	// The numbder of audio channels.
	uint audio_channels;

	// it can take more than one call to DecodeAudio() to extract all samples from an audio
	// packet, so store it locally til done.
	AVPacket audio_packet;
	uint8_t *audio_pkt_data;
	int audio_pkt_size;

	bool isReady = false;

	// Calls base-constructor, nothing else.
	VuoFfmpegDecoder(VuoUrl url);

	// Prepare internal stuff for decoding.
	bool Initialize();

	// Initialize the video context
	bool InitializeVideo(VuoFfmpegDecoder::AVContainer& container);

	// Initialize the audio context
	bool InitializeAudio(VuoFfmpegDecoder::AVContainer& container);

	// Read information about starting and ending timestamp + duration for both container.videoInfo & container.audioInfo
	bool InitializeVideoInfo();

	// Advance the next packet and place in either video or audio queue.
	bool NextPacket();

	// Decode a frame of audio, and store the frames in audioFrames list
	bool DecodeAudioFrame();

	// Decode a frame of video, and store the frames in videoFrames list
	bool DecodeVideoFrame();

	// Decode a chunk of video frames prior to the current video timestamp.
	bool DecodePreceedingVideoFrames();

	// Grab the next frame from audioFrames, calling NextPacket & DecodeAudio as necessary.
	bool FillAudioBuffer();

	// Seek to presentation timestamp.
	bool SeekToPts(int64_t pts);

	/// If currently seeking, this lets the decode audio/video functions know so they can skip
	/// unnecessary stuff
	bool seeking;

	// After av_seek, use this to step the decoded frames to a more exact position
	bool StepVideoFrame(int64_t pts);
	bool StepAudioFrame(int64_t pts);

	void ClearAudioBuffer();

	/**
	 * Audio sync notes -
	 * 	https://en.wikipedia.org/wiki/Audio_to_video_synchronization
	 * 	TV says -125ms to +45ms, Film is -22ms +22ms. (latency and lead, respectively)
	 * 	If audio is ahead, every other frame is dropped in DecodeVideoFrame(), and if audio is behind
	 * 	every other frame is repeated in NextVideoFrame().
	 */

	/// The maximum amount of time that the last sent audio timestamp can be ahead of the last sent video timestamp
	const double MAX_AUDIO_LEAD = .022;

	/// The maximum amount of time that the last sent audio timestamp can be behind the last sent video timestamp
	const double MAX_AUDIO_LATENCY = -.045;

	const unsigned int MAX_FRAME_SKIP = 1;

	/// Return the amount of audio drift in seconds (last sent audioTimestamp - videoTimestamp)
	double AudioOffset();
};

#ifdef __cplusplus
}
#endif
