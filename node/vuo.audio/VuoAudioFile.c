/**
 * @file
 * VuoAudio implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoAudioFile.h"
#include "VuoUrlFetch.h"
#include "VuoOsStatus.h"

#include <dispatch/dispatch.h>

#include <AudioToolbox/AudioToolbox.h>
//#include <CoreServices/CoreServices.h>
//#include <CoreAudio/CoreAudioTypes.h>
//#include <AudioToolbox/AudioFile.h>
//#include <AudioToolbox/AudioFormat.h>

#include "module.h"

#include "VuoWindow.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoAudioFile",
					 "dependencies" : [
						 "VuoAudioSamples",
						 "VuoInteger",
						 "VuoLoopType",
						 "VuoOsStatus",
						 "VuoReal",
						 "VuoText",
						 "VuoUrlFetch",
						 "VuoWindow",
						 "VuoList_VuoAudioSamples",
						 "AudioToolbox.framework",
						 "CoreFoundation.framework"
					 ]
				 });
#endif

/**
 * Context data for decoding an audio file.
 */
typedef struct VuoAudioFileInternal
{
	ExtAudioFileRef audioFile;							///< Core Audio file decoder.
	dispatch_queue_t audioFileQueue;					///< Serializes access to @c audioFile and @c decodedChannels.
	AudioStreamBasicDescription inputFormat;			///< The audio file's format.
	AudioStreamBasicDescription outputFormat;			///< The format to which we're asking Core Audio to convert the file.
	SInt64 totalFrameCount;								///< The number of frames in the audio file.
	SInt64 headerFrames;								///< The number of frames in the audio file's header (to be skipped when playing).

	bool playing;										///< Is the audio file currently playing?
	VuoLoopType loop;									///< What to do upon reaching the end of the audio file.

	void (*decodedChannels)(VuoList_VuoAudioSamples);	///< The trigger port callback, to be called when a new buffer is ready for playback.
	void (*finishedPlayback)(void);						///< The trigger port callback, to be called when the audio file has reached the end.
	dispatch_source_t playbackTimer;					///< Schedules decoding.
	dispatch_semaphore_t playbackTimerCanceled;			///< After @c playbackTimer has been canceled and executed its last decode, this is signaled.
} *VuoAudioFileInternal;

/**
 * Decodes a sample buffer for each channel, and fires the trigger port.
 */
static void VuoAudioFile_decodeChannels(VuoAudioFileInternal afi)
{
	if (!afi->playing)
		return;

	VuoInteger bufferSize = sizeof(float) * afi->outputFormat.mChannelsPerFrame * VuoAudioSamples_bufferSize;
	float *buffer = (float*)malloc(bufferSize);
	bzero(buffer, bufferSize);

	dispatch_sync(afi->audioFileQueue, ^{
		VuoInteger framesRemaining = VuoAudioSamples_bufferSize;
		float *subBuffer = buffer;

		while (framesRemaining)
		{
			AudioBufferList bufferList;
			bufferList.mNumberBuffers = 1;
			bufferList.mBuffers[0].mNumberChannels = afi->outputFormat.mChannelsPerFrame;
			bufferList.mBuffers[0].mDataByteSize = sizeof(float) * afi->outputFormat.mChannelsPerFrame * framesRemaining;
			bufferList.mBuffers[0].mData = (void *)subBuffer;

			UInt32 numFrames = framesRemaining;
			OSStatus err = ExtAudioFileRead(afi->audioFile, &numFrames, &bufferList);
			if (err != noErr)
			{
				free(buffer);
				char *errStr = VuoOsStatus_getText(err);
				VUserLog("Error reading samples: %s", errStr);
				free(errStr);
				afi->playing = false;
				if (afi->finishedPlayback)
					afi->finishedPlayback();
				return;
			}

			if (!numFrames)
			{
//				VLog("didn't get any more frames; breaking");
				break;
			}

			framesRemaining -= numFrames;
			subBuffer += numFrames * afi->outputFormat.mChannelsPerFrame;
		}

		if (framesRemaining)
		{
//			VLog("couldn't get all the frames; stopping playback");
			afi->playing = false;
			if (afi->finishedPlayback)
				afi->finishedPlayback();
		}
	});

	VuoList_VuoAudioSamples channels = VuoListCreate_VuoAudioSamples();

	for (VuoInteger channel = 0; channel < afi->outputFormat.mChannelsPerFrame; ++channel)
	{
		VuoAudioSamples samples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
		samples.samplesPerSecond = VuoAudioSamples_sampleRate;	///< @todo https://b33p.net/kosada/node/4726#comment-27512

		for (VuoInteger sample = 0; sample < VuoAudioSamples_bufferSize; ++sample)
			samples.samples[sample] = buffer[sample*afi->outputFormat.mChannelsPerFrame + channel];

		VuoListAppendValue_VuoAudioSamples(channels, samples);
	}
	free(buffer);

	dispatch_sync(afi->audioFileQueue, ^{
					  if (afi->decodedChannels)
						  afi->decodedChannels(channels);
					  else
					  {
						  VuoRetain(channels);
						  VuoRelease(channels);
					  }
				  });
}

/**
 * Deallocates the audio file decoder.
 */
void VuoAudioFile_free(void *af)
{
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;

	if (afi->playbackTimer)
	{
		dispatch_source_cancel(afi->playbackTimer);
		dispatch_release(afi->playbackTimer);
	}

	if (afi->playbackTimerCanceled)
	{
		dispatch_semaphore_wait(afi->playbackTimerCanceled, DISPATCH_TIME_FOREVER);
		dispatch_release(afi->playbackTimerCanceled);
	}

	if (afi->audioFileQueue)
		dispatch_release(afi->audioFileQueue);

	if (afi->audioFile)
		ExtAudioFileDispose(afi->audioFile);

	free(afi);
}

/**
 * Creates a new audio file decoder, and starts a decoding timer.
 */
VuoAudioFile VuoAudioFile_make(VuoText url)
{
	if (!url || url[0] == 0)
		return NULL;

	VuoAudioFileInternal afi = calloc(1, sizeof(struct VuoAudioFileInternal));
	VuoRegister(afi, VuoAudioFile_free);

	afi->playing = false;
	afi->loop = VuoLoopType_None;
	afi->decodedChannels = NULL;
	afi->finishedPlayback = NULL;

	{
		VuoText normalizedURL = VuoUrl_normalize(url, false);
		VuoRetain(normalizedURL);
		CFStringRef urlCFS = CFStringCreateWithCString(NULL, normalizedURL, kCFStringEncodingUTF8);
		CFURLRef url = CFURLCreateWithString(NULL, urlCFS, NULL);
		if (!url)
		{
			VUserLog("Couldn't open '%s': Invalid URL.", normalizedURL);
			VuoRelease(normalizedURL);
			CFRelease(urlCFS);
			goto fail;
		}

		// https://b33p.net/kosada/node/7971
		VuoApp_init();

		OSStatus err = ExtAudioFileOpenURL(url, &afi->audioFile);
		CFRelease(urlCFS);
		CFRelease(url);
		if (err != noErr)
		{
			char *errStr = VuoOsStatus_getText(err);
			VUserLog("Couldn't open '%s': %s", normalizedURL, errStr);
			free(errStr);
			VuoRelease(normalizedURL);
			goto fail;
		}
		VuoRelease(normalizedURL);
	}

	{
		UInt32 size = sizeof(afi->inputFormat);
		OSStatus err = ExtAudioFileGetProperty(afi->audioFile, kExtAudioFileProperty_FileDataFormat, &size, &afi->inputFormat);
		if (err != noErr)
		{
			char *errStr = VuoOsStatus_getText(err);
			VUserLog("Error getting file format for '%s': %s", url, errStr);
			free(errStr);
			goto fail;
		}
	}

	{
		UInt32 size = sizeof(afi->totalFrameCount);
		OSStatus err = ExtAudioFileGetProperty(afi->audioFile, kExtAudioFileProperty_FileLengthFrames, &size, &afi->totalFrameCount);
		if (err != noErr)
		{
			char *errStr = VuoOsStatus_getText(err);
			VUserLog("Error getting frame count for '%s': %s", url, errStr);
			free(errStr);
			goto fail;
		}
	}

	{
		afi->outputFormat = afi->inputFormat;
		afi->outputFormat.mFormatID = kAudioFormatLinearPCM;
		afi->outputFormat.mSampleRate = VuoAudioSamples_sampleRate;	///< @todo https://b33p.net/kosada/node/4726#comment-27512
		afi->outputFormat.mFormatFlags = kAudioFormatFlagIsFloat;
		afi->outputFormat.mBytesPerFrame = sizeof(float) * afi->outputFormat.mChannelsPerFrame;
		afi->outputFormat.mBitsPerChannel = sizeof(float) * 8;
		afi->outputFormat.mFramesPerPacket = 1;
		afi->outputFormat.mBytesPerPacket = afi->outputFormat.mBytesPerFrame * afi->outputFormat.mFramesPerPacket;
		UInt32 size = sizeof(afi->outputFormat);
		OSStatus err = ExtAudioFileSetProperty(afi->audioFile, kExtAudioFileProperty_ClientDataFormat, size, &afi->outputFormat);
		if (err != noErr)
		{
			char *errStr = VuoOsStatus_getText(err);
			VUserLog("Error setting output format for '%s': %s", url, errStr);
			free(errStr);
			goto fail;
		}
	}

	{
		AudioConverterRef ac;
		UInt32 size = sizeof(AudioConverterRef);
		bzero(&ac, size);
		OSStatus err = ExtAudioFileGetProperty(afi->audioFile, kExtAudioFileProperty_AudioConverter, &size, &ac);
		if (err != noErr)
		{
			char *errStr = VuoOsStatus_getText(err);
			VUserLog("Error getting audio converter info for '%s': %s", url, errStr);
			free(errStr);
		}
		else
		{
			AudioConverterPrimeInfo pi;
			UInt32 size = sizeof(AudioConverterPrimeInfo);
			bzero(&pi, size);
			OSStatus err = AudioConverterGetProperty(ac, kAudioConverterPrimeInfo, &size, &pi);
			if (err != noErr)
			{
				if (err != kAudioFormatUnsupportedPropertyError)
				{
					char *errStr = VuoOsStatus_getText(err);
					VUserLog("Error getting header info for '%s': %s", url, errStr);
					free(errStr);
				}
			}
			else
				afi->headerFrames = pi.leadingFrames;
		}
	}

	afi->audioFileQueue = dispatch_queue_create("org.vuo.audiofile", NULL);
	dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	afi->playbackTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, q);
	afi->playbackTimerCanceled = dispatch_semaphore_create(0);

	uint64_t nanoseconds = (float)VuoAudioSamples_bufferSize/VuoAudioSamples_sampleRate * NSEC_PER_SEC;	///< @todo https://b33p.net/kosada/node/4726#comment-27512
	dispatch_source_set_timer(afi->playbackTimer, dispatch_time(DISPATCH_TIME_NOW, nanoseconds), nanoseconds, 0);
	dispatch_source_set_event_handler(afi->playbackTimer, ^{
										  VuoAudioFile_decodeChannels(afi);
									  });
	dispatch_source_set_cancel_handler(afi->playbackTimer, ^{
										   dispatch_semaphore_signal(afi->playbackTimerCanceled);
									   });

	// Seek past the header.
	VuoAudioFile_setTime((VuoAudioFile)afi, 0);

	dispatch_resume(afi->playbackTimer);

	return (VuoAudioFile)afi;

fail:
	VuoRetain(afi);
	VuoRelease(afi);
	return NULL;
}

/**
 * Returns information about the specified audio file.
 *
 * Returns @c true on success, @c false if the file couldn't be read.
 */
bool VuoAudioFile_getInfo(VuoText url, VuoReal *duration, VuoInteger *channelCount, VuoReal *sampleRate)
{
	VuoAudioFileInternal afi = VuoAudioFile_make(url);
	if (!afi)
		return false;
	VuoRetain(afi);

	*duration = (float)afi->totalFrameCount / afi->inputFormat.mSampleRate;
	*channelCount = afi->inputFormat.mChannelsPerFrame;
	*sampleRate = afi->inputFormat.mSampleRate;

	VuoRelease(afi);

	return true;
}

/**
 * Specifies a trigger callback to invoke when a new audio sample buffer is ready for playback.
 *
 * Does not change playback status.
 */
void VuoAudioFile_enableTriggers(VuoAudioFile af, void (*decodedChannels)(VuoList_VuoAudioSamples), void (*finishedPlayback)(void))
{
	if (!af)
		return;
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;
	dispatch_sync(afi->audioFileQueue, ^{
					  afi->decodedChannels = decodedChannels;
					  afi->finishedPlayback = finishedPlayback;
				  });
}

/**
 * When this function returns, the trigger callback will no longer be called.
 *
 * Does not change playback status.
 */
void VuoAudioFile_disableTriggers(VuoAudioFile af)
{
	if (!af)
		return;
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;
	dispatch_sync(afi->audioFileQueue, ^{
					  afi->decodedChannels = NULL;
					  afi->finishedPlayback = NULL;
				  });
}

/**
 * Sets the behavior upon reaching the end of the audio file.
 */
void VuoAudioFile_setLoopType(VuoAudioFile af, VuoLoopType loop)
{
	if (!af)
		return;
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;
	afi->loop = loop;
}

/**
 * Seeks the playhead to the specified time in the audio file.
 */
void VuoAudioFile_setTime(VuoAudioFile af, VuoReal time)
{
	if (!af)
		return;
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;
	dispatch_sync(afi->audioFileQueue, ^{
					  SInt64 targetFrame = time * afi->inputFormat.mSampleRate;
//					  VLog("seeking to frame %lld (of %lld)", targetFrame, afi->totalFrameCount);
					  OSStatus err = ExtAudioFileSeek(afi->audioFile, targetFrame + afi->headerFrames);
					  if (err != noErr)
					  {
						  char *errStr = VuoOsStatus_getText(err);
						  VUserLog("Error seeking to sample: %s", errStr);
						  free(errStr);
						  return;
					  }
				  });
}

/**
 * Starts or resumes playback of the audio file from the current playhead time.
 */
void VuoAudioFile_play(VuoAudioFile af)
{
	if (!af)
		return;
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;
	afi->playing = true;
}

/**
 * Pauses playback of the audio file, leaving the playhead at its current time.
 */
void VuoAudioFile_pause(VuoAudioFile af)
{
	if (!af)
		return;
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;
	afi->playing = false;
}

/**
 * Returns @c true if the audio file is currently playing.
 *
 * This is independent of whether a trigger callback is currently active.
 */
bool VuoAudioFile_isPlaying(VuoAudioFile af)
{
	if (!af)
		return false;
	VuoAudioFileInternal afi = (VuoAudioFileInternal)af;
	return afi->playing;
}
