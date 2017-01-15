/**
 * @file
 * VuoQtListener implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoAvPlayerObject.h"
#include "VuoImageRenderer.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include "module.h"
#include "VuoGlPool.h"
#include "VuoVideoFrame.h"
#include <QuartzCore/CoreImage.h>
#include <Quartz/Quartz.h>
#include <QuartzCore/CVImageBuffer.h>
#include "VuoAvPlayerInterface.h"
#include <Accelerate/Accelerate.h>
#include "VuoWindow.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoAvPlayerObject",
					 "dependencies" : [
						"VuoImage",
						"VuoWindow",
						"AVFoundation.framework",
						"VuoImageRenderer",
						"CoreVideo.framework",
						"CoreMedia.framework",
						"VuoGLContext",
						"Accelerate.framework",

						// @todo
						"QuartzCore.framework",
						"Quartz.framework"

					 ]
				 });
#endif

/// The number of bytes to allocate initially for the audio buffer (all channels).  The actual buffer size may change
/// depending on what copyNextSampleBuffer feels like returning.
const unsigned int AUDIO_BUFFER_SIZE = 16384;

/// The number of preceeding frames to decode when playing video in reverse.
const unsigned int REVERSE_PLAYBACK_FRAME_ADVANCE = 10;

@implementation VuoAvPlayerObject

/**
 * Creates a new player.
 */
- (id) init
{
	// It seems we need an NSApplication instance to exist prior to using AVFoundation; otherwise, the app beachballs.
	// https://b33p.net/kosada/node/11006
	VuoApp_init();

	self = [super init];

	if (self)
	{
		glContext = VuoGlContext_use();
		CGLPixelFormatObj pf = VuoGlContext_makePlatformPixelFormat(false);
		CVReturn ret = CVOpenGLTextureCacheCreate(NULL, NULL, (CGLContextObj)glContext, pf, NULL, &textureCache);
		CGLReleasePixelFormat(pf);
		if (ret != kCVReturnSuccess)
		{
			VUserLog("Error: Couldn't create texture cache: %d", ret);
			return nil;
		}

		isReady = false;
		playOnReady = false;
		videoTimestamp = 0;
		audioTimestamp = 0;
		assetReader = nil;
		containsUnplayableAudio = false;

		readyToPlayCallback = NULL;
		avDecoderCppObject = NULL;

		videoQueue = [[NSMutableArray alloc] init];

		audioBufferCapacity = sizeof(Float32) * AUDIO_BUFFER_SIZE;
		audioBuffer = (Float32*) malloc(audioBufferCapacity);
	}

	return self;
}

+ (void) releaseAssetReader:(AVAssetReader*)reader
{
	AVAssetReader* readerToRelease = reader;

	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, .2 * NSEC_PER_SEC), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
	{
		if (readerToRelease.status == AVAssetReaderStatusReading)
			[readerToRelease cancelReading];

		[readerToRelease release];
	});
}

- (void) dealloc
{
	if(assetReader != nil)
		[VuoAvPlayerObject releaseAssetReader:assetReader];

	if(asset != nil)
	{
		[asset cancelLoading];
		[asset release];
	}

	readyToPlayCallback = NULL;
	avDecoderCppObject = NULL;

	CVOpenGLTextureCacheRelease(textureCache);
	VuoGlContext_disuse(glContext);

	[self clearFrameQueue];

	[videoQueue release];

	if(audioBuffer != nil)
		free(audioBuffer);

	[super dealloc];
}

/**
 * Loads media at `url`.
 */
- (bool) setURL:(NSURL*)url
{
	asset = [AVAsset assetWithURL:url];

	if(asset == nil)
	{
		VUserLog("VuoVideoDecoder: AvFoundation decoder could not find movie file at path: %s", [[url absoluteString] UTF8String]);
		return false;
	}

	[asset retain];

	if (![asset isPlayable] || [asset hasProtectedContent] || ![asset isReadable])
	{
		VUserLog("AvFoundation cannot play this asset.");
		isReady = false;
		return false;
	}

	NSArray* assetKeys = @[@"playable", @"hasProtectedContent", @"tracks"];

	[asset loadValuesAsynchronouslyForKeys: assetKeys completionHandler:^(void)
	{
		// First test whether the values of each of the keys we need have been successfully loaded.
		NSError *error = nil;
		bool failedLoading = false;

		for (NSString *key in assetKeys)
		{
			NSInteger status = [asset statusOfValueForKey:key error:&error];

			if(	error != nil ||
				status == AVKeyValueStatusFailed ||
				status == AVKeyValueStatusCancelled )
			{
				VUserLog("AVFoundation failed loading asset.");
				failedLoading = true;
				break;
			}
		}

		/// prevent race condition when trying to seek right away by enabling playback after initial setrange.
		isReady = !failedLoading && [self setAssetReaderTimeRange:CMTimeRangeMake(kCMTimeZero, [asset duration])];

		if(isReady)
		{
			/// try to load a few frames of video before declaring this asset ready to play.
			/// some codecs will load and successfully extract a single frame then crap out.
			/// *cough* mjpeg *cough*

			for(int i = 0; i < 3; i++)
			{
				 if(![self copyNextVideoSampleBuffer])
				 {
					VUserLog("AvFoundation successfully loaded but failed to extract a video buffer.");
				 	isReady = false;
				 	break;
				 }
			}
		}

		/// calling on main thread prevents tests from compiling in the event that isReady is false.
		// dispatch_async(dispatch_get_main_queue(), ^{
			if( self != nil && readyToPlayCallback != NULL && avDecoderCppObject != NULL )
				readyToPlayCallback( avDecoderCppObject, isReady );
		// });
	}];

	return true;
}

- (bool) canBeginPlayback
{
	return isReady;
}

- (bool) canPlayAudio
{
	return !containsUnplayableAudio;
}

- (double) getFrameRate
{
	return nominalFrameRate;
}

- (bool) setAssetReaderTimeRange:(CMTimeRange)timeRange
{
	NSError *error = nil;

	if(assetReader != nil)
	{
		[VuoAvPlayerObject releaseAssetReader:assetReader];

		assetReaderVideoTrackOutput = nil;
		assetReaderAudioTrackOutput = nil;
	}

	NSArray* videoTracks = [asset tracksWithMediaType:AVMediaTypeVideo];

	if ([videoTracks count] != 0)
	{
		AVAssetTrack* vidtrack = [videoTracks objectAtIndex:0];

        // for (id formatDescription in vidtrack.formatDescriptions)
        // {
        //     CMFormatDescriptionRef desc = (CMFormatDescriptionRef) formatDescription;
        //     CMVideoCodecType codec = CMFormatDescriptionGetMediaSubType(desc);

        //     /// jpeg files either don't play or have terrible performance with AVAssetReader,
        //     if(codec == kCMVideoCodecType_JPEG || codec == kCMVideoCodecType_JPEG_OpenDML)
        //     {
        //     	VUserLog("AVFoundation detected video codec is JPEG or MJPEG.");
        //     	return false;
        //     }
        // }

		nominalFrameRate = [vidtrack nominalFrameRate];

		assetReader = [AVAssetReader assetReaderWithAsset:asset error:&error];

		[assetReader retain];

		if(error) {
			VUserLog("AVAssetReader failed initialization: %s", [[error localizedDescription] UTF8String]);
			return false;
		}

		assetReader.timeRange = timeRange;

		/// video settings
		NSMutableDictionary * videoOutputSettings = [[[NSMutableDictionary alloc] init] autorelease];
		[videoOutputSettings setObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(NSString*)kCVPixelBufferPixelFormatTypeKey];
		[videoOutputSettings setObject:[NSNumber numberWithBool:YES] forKey:(NSString*)kCVPixelBufferOpenGLCompatibilityKey];

		assetReaderVideoTrackOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:vidtrack
			outputSettings:videoOutputSettings];

		if([assetReader canAddOutput:assetReaderVideoTrackOutput])
		{
			[assetReader addOutput:assetReaderVideoTrackOutput];
		}
		else
		{
			VUserLog("AVFoundation Video Decoder: assetReaderVideoTrackOutput cannot be added to assetReader.");
			return false;
		}

		/// audio settings

		SInt32 macMinorVersion;
		Gestalt(gestaltSystemVersionMinor, &macMinorVersion);

		NSDictionary* audioOutputSettings;

		// AVSampleRateKey is not supported on 10.7, and when not set can cause weird voice modulations
		if(macMinorVersion == 7)
		{
			audioOutputSettings = [NSDictionary dictionaryWithObjectsAndKeys:
									[NSNumber numberWithInt: kAudioFormatLinearPCM ], AVFormatIDKey,
									[NSNumber numberWithInt:32], AVLinearPCMBitDepthKey,
									[NSNumber numberWithBool:NO], AVLinearPCMIsBigEndianKey,
									[NSNumber numberWithBool:YES], AVLinearPCMIsFloatKey,
									[NSNumber numberWithBool:NO], AVLinearPCMIsNonInterleaved,
									nil];
		}
		else
		{
			audioOutputSettings = [NSDictionary dictionaryWithObjectsAndKeys:
									[NSNumber numberWithInt: kAudioFormatLinearPCM ], AVFormatIDKey,
									[NSNumber numberWithFloat: VuoAudioSamples_sampleRate], AVSampleRateKey,
									[NSNumber numberWithInt:32], AVLinearPCMBitDepthKey,
									[NSNumber numberWithBool:NO], AVLinearPCMIsBigEndianKey,
									[NSNumber numberWithBool:YES], AVLinearPCMIsFloatKey,
									[NSNumber numberWithBool:NO], AVLinearPCMIsNonInterleaved,
									nil];
		}

		// AVAssetTrack.playable isn't available pre-10.8, so just try to create the AVAssetTrackReaderOutput out
		// of every audio track until either one sticks or we run out of tracks.
		for(AVAssetTrack* track in [asset tracksWithMediaType:AVMediaTypeAudio])
		{
			if(track == nil)
				continue;

			audioChannelCount = [self getAudioChannelCount:track];

			if(audioChannelCount < 1)
			{
				containsUnplayableAudio = true;
				continue;
			}

			assetReaderAudioTrackOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:track outputSettings:audioOutputSettings];

			if(assetReaderAudioTrackOutput != nil && [assetReader canAddOutput:assetReaderAudioTrackOutput])
			{
				[assetReader addOutput:assetReaderAudioTrackOutput];
				containsUnplayableAudio = false;
				break;
			}
			else
			{
				VUserLog("AVFoundation Video Decoder: AssetReaderAudioTrackOutput cannot be added to assetReader.");
				audioChannelCount = 0;
				containsUnplayableAudio = true;
				continue;
			}
		}

		return [assetReader startReading];
	}
	else
	{
		VUserLog("No video track found!");
		return false;
	}
}

- (unsigned int) getAudioChannelCount:(AVAssetTrack*) track
{
	NSArray* formatDesc = track.formatDescriptions;

	for(unsigned int i = 0; i < [formatDesc count]; ++i)
	{
		CMAudioFormatDescriptionRef item = (CMAudioFormatDescriptionRef)[formatDesc objectAtIndex:i];
		const AudioStreamBasicDescription* audioDescription = CMAudioFormatDescriptionGetStreamBasicDescription (item);

		if(audioDescription != nil)
		{
			return audioDescription->mChannelsPerFrame;
		}
	}

	return 0;
}

- (void) setPlaybackRate:(double)rate
{
	/// playback is swapping directions, clear the caches
	if(playbackRate < 0 != rate < 0)
	{
		// if was reverse and now is forward
		if(rate > 0)
			[self seekToSecond:videoTimestamp withRange:-1];
		else
			[self clearFrameQueue];
	}

	playbackRate = rate;
}

- (bool) audioEnabled
{
	return VuoReal_areEqual(playbackRate, 1);
}

- (unsigned int) audioChannels
{
	return audioChannelCount;
}

- (void) clearFrameQueue
{
	for(int i = 0; i < [videoQueue count]; i++)
	{
		VuoVideoFrame* frame = [[videoQueue objectAtIndex:i] pointerValue];
		VuoVideoFrame_release(*frame);
		free(frame);
	}

	[videoQueue removeAllObjects];

	audioBufferSampleIndex = 0;
	audioBufferSamplesPerChannel = 0;
}

- (bool) seekToSecond:(float)second withRange:(float)range
{
	if(![self canBeginPlayback])
		return false;

	[self clearFrameQueue];

	CMTime cmsec = CMTimeMakeWithSeconds(second, NSEC_PER_SEC);

	[self setAssetReaderTimeRange: CMTimeRangeMake(cmsec, range < 0 ? [asset duration] : CMTimeMakeWithSeconds(range, NSEC_PER_SEC))];

	// read one sample to get an accurate timestamp
	[self copyNextVideoSampleBuffer];
	[self copyNextAudioSampleBuffer];

	if([videoQueue count] > 0)
	{
		videoTimestamp = ((VuoVideoFrame*)[[videoQueue objectAtIndex:0] pointerValue])->timestamp;
	}
	else if(second >= [self getDuration])
	{
		videoTimestamp = [self getDuration];
	}

	return true;
}

- (void) setPlayerCallback:(void (*)(void* functionPtr, bool canPlayMedia))callback target:(void*)decoderCppObject
{
	readyToPlayCallback = callback;
	avDecoderCppObject = decoderCppObject;
}

- (VuoReal) getCurrentTimestamp
{
	return (VuoReal) videoTimestamp;
}

/// Returns the current frame's image.
- (bool) nextVideoFrame:(VuoVideoFrame*)frame
{
	if([videoQueue count] < 1)
	{
		if( playbackRate < 0)
		{
			if(![self decodePreceedingVideoSamples])
				return false;
		}
		else
		{
			if(![self copyNextVideoSampleBuffer] )
				return false;
		}
	}

	int index = playbackRate < 0 ? [videoQueue count] - 1 : 0;

	NSValue* value = [videoQueue objectAtIndex:index];
	VuoVideoFrame* framePointer = [value pointerValue];
	*frame = *framePointer;
	videoTimestamp = frame->timestamp;
	[videoQueue removeObjectAtIndex:index];

	free(framePointer);

	return true;
}

- (bool) decodePreceedingVideoSamples
{
	float rewindIncrement = (1./nominalFrameRate * REVERSE_PLAYBACK_FRAME_ADVANCE);

	if(videoTimestamp <= 0.)
		return false;

	float seek = fmax(0, videoTimestamp - rewindIncrement);
	[self seekToSecond:seek withRange:rewindIncrement];

	while([self copyNextVideoSampleBuffer]) {}

	return [videoQueue count] > 0;
}

- (VuoReal) getDuration
{
	return (VuoReal) CMTimeGetSeconds([asset duration]);
}

- (bool) nextAudioFrame:(VuoAudioFrame*) frame
{
	if(![self audioEnabled])
		return false;

	int sampleIndex = 0;
	int sampleCapacity = VuoAudioSamples_bufferSize;

	for(int i = 0; i < audioChannelCount; i++)
	{
		VuoAudioSamples samples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
		samples.samplesPerSecond = VuoAudioSamples_sampleRate;
		VuoListAppendValue_VuoAudioSamples(frame->samples, samples);
	}

	while(sampleIndex < sampleCapacity - 1)
	{
		if(audioBufferSampleIndex >= audioBufferSamplesPerChannel)
		{
			if(![self copyNextAudioSampleBuffer])
			{
				return false;
			}
			else
			{
				/// this shouldn't ever happen, but if it does realloc samples and
				/// change the audioChannelCount size
				if(audioChannelCount != audioBufferChannelCount)
				{
					VuoRelease(frame->samples);
					frame->samples = NULL;
					const VuoAudioSamples empty = {0, NULL, 48000};
					frame->samples = VuoListCreateWithCount_VuoAudioSamples(audioBufferChannelCount, empty);
					VuoRetain(frame->samples);

					for(int i = 0; i < audioBufferChannelCount; i++)
					{
						VuoAudioSamples audioSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
						audioSamples.samplesPerSecond = VuoAudioSamples_sampleRate;
						VuoListAppendValue_VuoAudioSamples(frame->samples, audioSamples);
					}

					audioChannelCount = audioBufferChannelCount;
				}
			}
		}

		unsigned int copyLength = MIN(sampleCapacity - sampleIndex, audioBufferSamplesPerChannel - audioBufferSampleIndex);

		for(int i = 0; i < audioChannelCount; i++)
		{
			VuoReal* frame_samples = VuoListGetValue_VuoAudioSamples(frame->samples, i+1).samples;
			int frame_sample_index = sampleIndex, buffer_sample_index = audioBufferSampleIndex;

			for(int iterator = 0; iterator < copyLength; iterator++)
			{
				frame_samples[frame_sample_index] = audioBuffer[buffer_sample_index * audioChannelCount + i];

				frame_sample_index++;
				buffer_sample_index++;
			}
		}

		sampleIndex += copyLength;
		audioBufferSampleIndex += copyLength;
	}

	frame->timestamp = audioTimestamp;

	return true;
}

/**
 * Does nothing.
 */
static void VuoAvPlayerObject_freeCallback(VuoImage imageToFree)
{
}

/// read the next video sample buffer
- (bool) copyNextVideoSampleBuffer
{
	if( [assetReader status] == AVAssetReaderStatusReading )
	{
		CMSampleBufferRef sampleBuffer = [assetReaderVideoTrackOutput copyNextSampleBuffer];

		if (!sampleBuffer)
			return false;

		float timestamp = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer));

		CVPixelBufferRef buffer = (CVPixelBufferRef) CMSampleBufferGetImageBuffer(sampleBuffer);

		CVOpenGLTextureRef texture;
		CVReturn ret = CVOpenGLTextureCacheCreateTextureFromImage(NULL, textureCache, buffer, NULL, &texture);
		if (ret != kCVReturnSuccess)
		{
			VUserLog("Error: %d", ret);
			CMSampleBufferInvalidate(sampleBuffer);
			CFRelease(sampleBuffer);
			return false;
		}
		VuoImage rectImage = VuoImage_makeClientOwnedGlTextureRectangle(
					CVOpenGLTextureGetName(texture),
					GL_RGB,
					CVPixelBufferGetWidth(buffer),
					CVPixelBufferGetHeight(buffer),
					VuoAvPlayerObject_freeCallback, NULL);
		VuoRetain(rectImage);
		VuoImage image = VuoImage_makeCopy(rectImage, CVOpenGLTextureIsFlipped(texture));
		CVOpenGLTextureRelease(texture);
		VuoRelease(rectImage);
		CVOpenGLTextureCacheFlush(textureCache, 0);

		CMSampleBufferInvalidate(sampleBuffer);
		CFRelease(sampleBuffer);
		sampleBuffer = nil;

		VuoVideoFrame* frame = (VuoVideoFrame*) malloc(sizeof(VuoVideoFrame));
		*frame = VuoVideoFrame_make(image, timestamp);
		VuoVideoFrame_retain(*frame);
		NSValue* val = [NSValue valueWithPointer:frame];
		[videoQueue addObject:val];

		return true;
	}
	else if ([assetReader status] == AVAssetReaderStatusFailed)
	{
		VUserLog("Error: AVAssetReader failed: %s. %s.",
				 [[[assetReader error] localizedDescription] UTF8String],
				 [[[assetReader error] localizedFailureReason] UTF8String]);
		return false;
	}
	else
	{
		VUserLog("Error: AVAssetReader status %ld", [assetReader status]);
		return false;
	}
}

- (bool) copyNextAudioSampleBuffer
{
	if([assetReader status] == AVAssetReaderStatusReading)
	{
		CMSampleBufferRef audioSampleBuffer = [assetReaderAudioTrackOutput copyNextSampleBuffer];

		audioBufferSampleIndex = 0;

		if(audioSampleBuffer == NULL)
		{
			audioBufferSamplesPerChannel = 0;
			return false;
		}

		// CMItemCount numSamplesInAudioBuffer = CMSampleBufferGetNumSamples(audioSampleBuffer);
		audioTimestamp = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(audioSampleBuffer));

		CMBlockBufferRef audioBlockBuffer = CMSampleBufferGetDataBuffer( audioSampleBuffer );
		// AudioBufferList audioBufferList;

		size_t bufferSize = 0;

		CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
			audioSampleBuffer,
			&bufferSize,
			NULL,
			0,
			NULL,
			NULL,
			0,
			NULL
		);

		AudioBufferList* audioBufferList = malloc(bufferSize);

		OSStatus err = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
			audioSampleBuffer,
			NULL,
			audioBufferList,
			bufferSize,
			NULL,
			NULL,
			kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
			&audioBlockBuffer
		);

		if(err != 0)
		{
			VUserLog("AvFoundation failed extracting audio buffer: %i", err);

			audioBufferSamplesPerChannel = 0;
			free(audioBufferList);
			CFRelease(audioSampleBuffer);

			return false;
		}

		// CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
		// 	audioSampleBuffer,
		// 	NULL,
		// 	&audioBufferList,
		// 	sizeof(audioBufferList),
		// 	NULL,
		// 	NULL,
		// 	kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
		// 	&audioBlockBuffer);

		// I haven't been able to find anything about when multiple buffers would be used.  In every format
		// so far there's only ever one, with channels interleaved in mData.  I wasn't sure whether multiple
		// buffers would mean additional channels, or more data for the same channels, or a different track
		// entirely.  So for now just look at the first buffer and check here first when something inevitably
		// goes wrong.

		// for (int curBuffer = 0; curBuffer < audioBufferList.mNumberBuffers; curBuffer++)
		const int curBuffer = 0;

		audioBufferChannelCount = audioBufferList->mBuffers[curBuffer].mNumberChannels;
		size_t dataByteSize = audioBufferList->mBuffers[curBuffer].mDataByteSize;
		audioBufferSamplesPerChannel = (dataByteSize / sizeof(Float32)) / audioBufferChannelCount;

		if( dataByteSize > audioBufferCapacity )
		{
			if (!(audioBuffer = realloc(audioBuffer, dataByteSize)))
			{
				free(audioBufferList);
				CFRelease(audioSampleBuffer);
				audioBufferSamplesPerChannel = 0;
				VUserLog("AvFoundation video decoder is out of memory.");
				return false;
			}

			audioBufferCapacity = dataByteSize;
		}

		Float32* samples = (Float32 *)audioBufferList->mBuffers[curBuffer].mData;
		memcpy(audioBuffer, samples, audioBufferList->mBuffers[curBuffer].mDataByteSize);

		free(audioBufferList);
		CFRelease(audioSampleBuffer);

		return true;
	}
	return false;
}

@end
