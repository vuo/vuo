/**
 * @file
 * VuoQtListener implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoAvPlayerObject.h"
#include "VuoCompositionState.h"
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
#include "VuoOsStatus.h"
#include "VuoApp.h"
#include "HapInAVFoundation.h"

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
						"VuoGlContext",
						"Accelerate.framework",
						"VuoOsStatus",

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


/**
 * Allows us to call methods on HapDecoderFrame without linking to the framework at compile-time
 * (to enable running on OS X 10.8 and 10.9).
 */
@protocol VuoAvTrackHapFrame
/// @{ Stub.
@property (readonly) OSType codecSubType;
@property (readonly) int dxtPlaneCount;
@property (readonly) void** dxtDatas;
@property (readonly) OSType* dxtPixelFormats;
@property (readonly) NSSize dxtImgSize;
@property (readonly) CMTime presentationTime;
/// @}
@end

/**
 * Allows us to call methods on AVPlayerItemHapDXTOutput without linking to the framework at compile-time
 * (to enable running on OS X 10.8 and 10.9).
 */
@protocol VuoAvTrackHapOutput
/// @{ Stub.
- (id)initWithHapAssetTrack:(AVAssetTrack *)track;
- (NSObject<VuoAvTrackHapFrame> *) allocFrameForHapSampleBuffer:(CMSampleBufferRef)n;
/// @}
@end

/**
 * Decodes Hap frames if present; otherwise returns the frames as-is.
 */
@interface VuoAvTrackOutput : AVAssetReaderTrackOutput
{
	NSObject<VuoAvTrackHapOutput> *hapOutput;	///< If non-nil, this track contains Hap frames, and this object lets us decode them.
}
@end

@implementation VuoAvTrackOutput

- (id)initWithTrack:(AVAssetTrack *)track outputSettings:(NSDictionary *)settings
{
	bool isHap = false;
	for (id formatDescription in track.formatDescriptions)
	{
		CMFormatDescriptionRef desc = (CMFormatDescriptionRef) formatDescription;
		CMVideoCodecType codec = CMFormatDescriptionGetMediaSubType(desc);
		if (codec == 'Hap1'
		 || codec == 'Hap5'
		 || codec == 'HapY'
		 || codec == 'HapM')
		{
			isHap = true;
			break;
		}
	}

	if (self = [super initWithTrack:track outputSettings:(isHap ? nil : settings)])
	{
		if (isHap)
		{
			NSBundle *f = [NSBundle bundleWithPath:[NSString stringWithFormat:@"%s/Vuo.framework/Versions/%s/Frameworks/HapInAVFoundation.framework",
																			  VuoGetFrameworkPath(),
																			  VUO_FRAMEWORK_VERSION_STRING]];
			if (!f)
			{
				f = [NSBundle bundleWithPath:[NSString stringWithFormat:@"%s/VuoRunner.framework/Versions/%s/Frameworks/HapInAVFoundation.framework",
																		VuoGetRunnerFrameworkPath(),
																		VUO_FRAMEWORK_VERSION_STRING]];
				if (!f)
				{
					VUserLog("Error: Playing this movie requires HapInAVFoundation.framework, but I can't find it.");
					return nil;
				}
			}

			if (![f isLoaded])
			{
				NSError *error;
				bool status = [f loadAndReturnError:&error];
				if (!status)
				{
					NSError *underlyingError = [[error userInfo] objectForKey:NSUnderlyingErrorKey];
					if (underlyingError)
						error = underlyingError;
					VUserLog("Error: Playing this movie requires HapInAVFoundation.framework, but it wouldn't load: %s", [[error localizedDescription] UTF8String]);
					return nil;
				}
			}

			hapOutput = [[NSClassFromString(@"AVPlayerItemHapDXTOutput") alloc] initWithHapAssetTrack:track];
		}
	}
	return self;
}

- (NSObject<VuoAvTrackHapFrame> *)newHapFrame
{
	if (!hapOutput)
		return nil;

	CMSampleBufferRef buffer = [super copyNextSampleBuffer];
	if (!buffer)
		return nil;
	VuoDefer(^{ CFRelease(buffer); });

	return [hapOutput allocFrameForHapSampleBuffer:buffer];
}

- (void)dealloc
{
	[hapOutput release];
	[super dealloc];
}

@end


@implementation VuoAvPlayerObject

/**
 * Creates a new player.
 */
- (id) init
{
	// It seems we need an NSApplication instance to exist prior to using AVFoundation; otherwise, the app beachballs.
	// https://b33p.net/kosada/node/11006
	VuoApp_init(false);

	self = [super init];

	if (self)
	{
		CGLPixelFormatObj pf = VuoGlContext_makePlatformPixelFormat(false, false, -1);
		__block CVReturn ret;
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			ret = CVOpenGLTextureCacheCreate(NULL, NULL, cgl_ctx, pf, NULL, &textureCache);
		});
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

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		CVOpenGLTextureCacheRelease(textureCache);
	});

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

	// Movies using the Hap codec are not "playable" yet we can still play them.
	// Also, https://developer.apple.com/videos/play/wwdc2017/511?time=293 says `isPlayable`
	// means that the current hardware is capable of playing back the current media in realtime,
	// but we can ignore that since we want to try to play regardless.
	bool isPlayable = true; //[asset isPlayable];

	// "Currently, only Apple can 'make' protected content and applications that can play it."
	// http://www.cocoabuilder.com/archive/cocoa/301980-avfoundation-authorization-for-playback-of-protected-content.html#301981
	bool isProtected = [asset hasProtectedContent];

	if (!isPlayable || isProtected || ![asset isReadable])
	{
		VUserLog("AvFoundation cannot play this asset (isPlayable=%d, hasProtectedContent=%d, isReadable=%d).",
				 [asset isPlayable], [asset hasProtectedContent], [asset isReadable]);
		isReady = false;
		return false;
	}


	{
		AVAssetTrack *videoTrack = [asset tracksWithMediaType:AVMediaTypeVideo][0];
		CMFormatDescriptionRef desc = (CMFormatDescriptionRef)videoTrack.formatDescriptions[0];
		CMVideoCodecType codec = CMFormatDescriptionGetMediaSubType(desc);
		if (codec == 'ap4h'  // AVVideoCodecTypeAppleProRes4444
		 || codec == 'Hap5'  // Hap Alpha
		 || codec == 'HapM'  // Hap Q Alpha
		 || codec == 'HapA') // Hap Alpha-only
			hasAlpha = true;
		if (codec == 'hvc1') // HEVC movies may have an alpha attachment.
		{
			CFBooleanRef containsAlphaChannel = CMFormatDescriptionGetExtension(desc, CFSTR("ContainsAlphaChannel"));  // kCMFormatDescriptionExtension_ContainsAlphaChannel (macOS 10.15+)
			if (containsAlphaChannel == kCFBooleanTrue)
				hasAlpha = true;
		}

		if (VuoIsDebugEnabled())
		{
			char *codecZ = VuoOsStatus_getText(codec);
			int bitsPerComponent = ((NSNumber *)CMFormatDescriptionGetExtension(desc, CFSTR("BitsPerComponent"))).intValue;
			int depth            = ((NSNumber *)CMFormatDescriptionGetExtension(desc, CFSTR("Depth"))).intValue;

			VuoText codecName = VuoText_makeFromCFString(CMFormatDescriptionGetExtension(desc, CFSTR("FormatName")));
			VuoRetain(codecName);
			VuoText alphaMode = VuoText_makeFromCFString(CMFormatDescriptionGetExtension(desc, CFSTR("AlphaChannelMode")));
			VuoRetain(alphaMode);
			VUserLog("codec=\"%s\" (%s)  bpc=%d  depth=%d  hasAlpha=%d %s",
					 codecName,
					 codecZ,
					 bitsPerComponent,
					 depth,
					 hasAlpha,
					 (hasAlpha && alphaMode) ? alphaMode : "");
			VuoRelease(codecName);
			VuoRelease(alphaMode);
			free(codecZ);
		}
	}


	NSArray* assetKeys = @[@"playable", @"hasProtectedContent", @"tracks"];

	void *compositionState = vuoCopyCompositionStateFromThreadLocalStorage();
	[asset loadValuesAsynchronouslyForKeys: assetKeys completionHandler:^(void)
	{
		vuoAddCompositionStateToThreadLocalStorage(compositionState);

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

		vuoRemoveCompositionStateFromThreadLocalStorage();
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

		NSDictionary *videoOutputSettings = @{
			(NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
			(NSString *)kCVPixelBufferOpenGLCompatibilityKey: @(YES),
		};

		assetReaderVideoTrackOutput = [VuoAvTrackOutput assetReaderTrackOutputWithTrack:vidtrack
			outputSettings:videoOutputSettings];
		if (!assetReaderVideoTrackOutput)
			return false;

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

		NSDictionary* audioOutputSettings;

			audioOutputSettings = [NSDictionary dictionaryWithObjectsAndKeys:
									[NSNumber numberWithInt: kAudioFormatLinearPCM ], AVFormatIDKey,
									[NSNumber numberWithFloat: VuoAudioSamples_sampleRate], AVSampleRateKey,
									[NSNumber numberWithInt:32], AVLinearPCMBitDepthKey,
									[NSNumber numberWithBool:NO], AVLinearPCMIsBigEndianKey,
									[NSNumber numberWithBool:YES], AVLinearPCMIsFloatKey,
									[NSNumber numberWithBool:NO], AVLinearPCMIsNonInterleaved,
									nil];

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
			[self seekToSecond:videoTimestamp withRange:-1 frame:NULL];
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

- (bool) seekToSecond:(float)second withRange:(float)range frame:(VuoVideoFrame *)frame
{
	if(![self canBeginPlayback])
		return false;

	[self clearFrameQueue];

	if (playbackRate < 0 && VuoReal_areEqual(second, CMTimeGetSeconds(asset.duration)))
	{
		float preroll = (1./nominalFrameRate * REVERSE_PLAYBACK_FRAME_ADVANCE);
		second -= preroll;
	}

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

	if (frame)
		if (![self nextVideoFrame:frame])
		{
			frame->image = NULL;
			frame->timestamp = 0;
			frame->duration = 0;
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

		if ([videoQueue count] < 1)
			return false;
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
	[self seekToSecond:seek withRange:rewindIncrement frame:NULL];

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
		VuoListAppendValue_VuoAudioSamples(frame->channels, samples);
	}

	while(sampleIndex < sampleCapacity - 1)
	{
		if(audioBufferSampleIndex >= audioBufferSamplesPerChannel)
		{
			if(![self copyNextAudioSampleBuffer])
			{
				return false;
			}
		}

		unsigned int copyLength = MIN(sampleCapacity - sampleIndex, audioBufferSamplesPerChannel - audioBufferSampleIndex);

		for(int i = 0; i < audioChannelCount; i++)
		{
			VuoReal* frame_samples = VuoListGetValue_VuoAudioSamples(frame->channels, i+1).samples;
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
		__block VuoImage image = NULL;
		float timestamp;
		float duration;
		NSObject<VuoAvTrackHapFrame> *hapFrame = [assetReaderVideoTrackOutput newHapFrame];
		if (hapFrame)
		{
			// For Hap frames, get the DXT buffer in CPU memory, and upload it to the GPU.

			timestamp = CMTimeGetSeconds(hapFrame.presentationTime);
			duration = 1./nominalFrameRate;
			int dxtPlaneCount = hapFrame.dxtPlaneCount;
			OSType *dxtPixelFormats = hapFrame.dxtPixelFormats;
			void **dxtBaseAddresses = hapFrame.dxtDatas;

			if (dxtPlaneCount > 2)
			{
				VUserLog("Error: This image has %d planes, which isn't part of the Hap spec.", dxtPlaneCount);
				return false;
			}

			VuoGlContext_perform(^(CGLContextObj cgl_ctx){
				VuoImage dxtImage[dxtPlaneCount];
				for (int i = 0; i < dxtPlaneCount; ++i)
				{
					GLuint internalFormat;
					unsigned int bitsPerPixel;
					switch (dxtPixelFormats[i])
					{
						case kHapCVPixelFormat_RGB_DXT1:
							internalFormat = HapTextureFormat_RGB_DXT1;
							bitsPerPixel = 4;
							break;

						case kHapCVPixelFormat_RGBA_DXT5:
						case kHapCVPixelFormat_YCoCg_DXT5:
							internalFormat = HapTextureFormat_RGBA_DXT5;
							bitsPerPixel = 8;
							break;

						case kHapCVPixelFormat_CoCgXY:
							if (i == 0)
							{
								internalFormat = HapTextureFormat_RGBA_DXT5;
								bitsPerPixel = 8;
							}
							else
							{
								internalFormat = HapTextureFormat_A_RGTC1;
								bitsPerPixel = 4;
							}
							break;

						case kHapCVPixelFormat_YCoCg_DXT5_A_RGTC1:
							if (i == 0)
							{
								internalFormat = HapTextureFormat_RGBA_DXT5;
								bitsPerPixel = 8;
							}
							else
							{
								internalFormat = HapTextureFormat_A_RGTC1;
								bitsPerPixel = 4;
							}
							break;

						case kHapCVPixelFormat_A_RGTC1:
							internalFormat = HapTextureFormat_A_RGTC1;
							bitsPerPixel = 4;
							break;

						default:
							VUserLog("Error: Unknown Hap dxtPixelFormat %s.", VuoOsStatus_getText(dxtPixelFormats[i]));
							return;
					}

					GLuint texture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_NoAllocation, GL_TEXTURE_2D, internalFormat, hapFrame.dxtImgSize.width, hapFrame.dxtImgSize.height, GL_RGBA, NULL);
					glBindTexture(GL_TEXTURE_2D, texture);

					size_t bytesPerRow = (hapFrame.dxtImgSize.width * bitsPerPixel) / 8;
					GLsizei dataLength = (int)(bytesPerRow * hapFrame.dxtImgSize.height);

					glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);

					glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, hapFrame.dxtImgSize.width, hapFrame.dxtImgSize.height, 0, dataLength, dxtBaseAddresses[i]);

					glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
					glBindTexture(GL_TEXTURE_2D, 0);

					dxtImage[i] = VuoImage_make(texture, internalFormat, hapFrame.dxtImgSize.width, hapFrame.dxtImgSize.height);
				}

				if (hapFrame.codecSubType == kHapCodecSubType
				 || hapFrame.codecSubType == kHapAlphaCodecSubType)
					image = VuoImage_makeCopy(dxtImage[0], true, 0, 0, false);
				else if (hapFrame.codecSubType == kHapYCoCgCodecSubType)
				{
					// Based on https://github.com/Vidvox/hap-in-avfoundation/blob/master/HapInAVF%20Test%20App/ScaledCoCgYToRGBA.frag
					static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
						uniform sampler2D cocgsy_src;
						const vec4 offsets = vec4(-0.50196078431373, -0.50196078431373, 0.0, 0.0);
						varying vec2 fragmentTextureCoordinate;
						void main()
						{
							vec4 CoCgSY = texture2D(cocgsy_src, vec2(fragmentTextureCoordinate.x, 1. - fragmentTextureCoordinate.y));

							CoCgSY += offsets;

							float scale = ( CoCgSY.z * ( 255.0 / 8.0 ) ) + 1.0;

							float Co = CoCgSY.x / scale;
							float Cg = CoCgSY.y / scale;
							float Y = CoCgSY.w;

							gl_FragColor = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);
						}
					);

					VuoShader shader = VuoShader_make("YCoCg to RGB Shader");
					VuoRetain(shader);
					VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
					VuoShader_setUniform_VuoImage(shader, "cocgsy_src", dxtImage[0]);
					image = VuoImageRenderer_render(shader, dxtImage[0]->pixelsWide, dxtImage[0]->pixelsHigh, VuoImageColorDepth_8);
					VuoRelease(shader);
				}
				else if (hapFrame.codecSubType == kHapYCoCgACodecSubType)
				{
					// Based on https://github.com/Vidvox/hap-in-avfoundation/blob/master/HapInAVF%20Test%20App/ScaledCoCgYPlusAToRGBA.frag
					static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
						uniform sampler2D cocgsy_src;
						uniform sampler2D alpha_src;
						const vec4 offsets = vec4(-0.50196078431373, -0.50196078431373, 0.0, 0.0);
						varying vec2 fragmentTextureCoordinate;
						void main()
						{
							vec2 tc = vec2(fragmentTextureCoordinate.x, 1. - fragmentTextureCoordinate.y);
							vec4 CoCgSY = texture2D(cocgsy_src, tc);
							float alpha = texture2D(alpha_src, tc).r;

							CoCgSY += offsets;

							float scale = ( CoCgSY.z * ( 255.0 / 8.0 ) ) + 1.0;

							float Co = CoCgSY.x / scale;
							float Cg = CoCgSY.y / scale;
							float Y = CoCgSY.w;

							gl_FragColor = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, alpha);
						}
					);

					VuoShader shader = VuoShader_make("YCoCg+A to RGBA Shader");
					VuoRetain(shader);
					VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
					VuoShader_setUniform_VuoImage(shader, "cocgsy_src", dxtImage[0]);
					VuoShader_setUniform_VuoImage(shader, "alpha_src", dxtImage[1]);
					image = VuoImageRenderer_render(shader, dxtImage[0]->pixelsWide, dxtImage[0]->pixelsHigh, VuoImageColorDepth_8);
					VuoRelease(shader);
				}
				else
				{
					VUserLog("Error: Unknown codecSubType '%s'.", VuoOsStatus_getText(hapFrame.codecSubType));
					return;
				}
			});

			[hapFrame release];

			if (!image)
				return false;
		}
		else
		{
			// For non-Hap frames, AV Foundation can directly give us an OpenGL texture.

			CMSampleBufferRef sampleBuffer = [assetReaderVideoTrackOutput copyNextSampleBuffer];

			if (!sampleBuffer)
				return false;

			timestamp = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer));
			duration  = CMTimeGetSeconds(CMSampleBufferGetDuration(sampleBuffer));
			if (isnan(duration))
				duration = 1./nominalFrameRate;

			CVPixelBufferRef buffer = (CVPixelBufferRef) CMSampleBufferGetImageBuffer(sampleBuffer);
			if (!buffer)
			{
				CMSampleBufferInvalidate(sampleBuffer);
				CFRelease(sampleBuffer);
				return false;
			}

			__block CVOpenGLTextureRef texture;
			__block CVReturn ret;
			VuoGlContext_perform(^(CGLContextObj cgl_ctx){
				ret = CVOpenGLTextureCacheCreateTextureFromImage(NULL, textureCache, buffer, NULL, &texture);
			});
			if (ret != kCVReturnSuccess)
			{
				char *error = VuoOsStatus_getText(ret);
				VUserLog("Error: Couldn't convert CVImageBuffer to texture: %s", error);
				free(error);
				CMSampleBufferInvalidate(sampleBuffer);
				CFRelease(sampleBuffer);
				return false;
			}
			VuoImage rectImage = VuoImage_makeClientOwnedGlTextureRectangle(
						CVOpenGLTextureGetName(texture),
						hasAlpha ? GL_RGBA : GL_RGB,
						CVPixelBufferGetWidth(buffer),
						CVPixelBufferGetHeight(buffer),
						VuoAvPlayerObject_freeCallback, NULL);
			VuoRetain(rectImage);
			image = VuoImage_makeCopy(rectImage, CVOpenGLTextureIsFlipped(texture), 0, 0, false);
			CVOpenGLTextureRelease(texture);
			VuoRelease(rectImage);
			VuoGlContext_perform(^(CGLContextObj cgl_ctx){
				CVOpenGLTextureCacheFlush(textureCache, 0);
			});

			CMSampleBufferInvalidate(sampleBuffer);
			CFRelease(sampleBuffer);
		}

		VuoVideoFrame* frame = (VuoVideoFrame*) malloc(sizeof(VuoVideoFrame));
		*frame = VuoVideoFrame_make(image, timestamp, duration);
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


		// Sometimes AVFoundation says the movie has a certain number of audio channels,
		// but actually decodes a different number of audio channels.
		// https://b33p.net/kosada/node/12952
		if (audioChannelCount != audioBufferChannelCount)
		{
			VUserLog("Warning: AVFoundation reported %d audio channel%s, but actually decoded %d channel%s.",
					 audioChannelCount, audioChannelCount == 1 ? "" : "s",
					 audioBufferChannelCount, audioBufferChannelCount == 1 ? "" : "s");
			audioChannelCount = audioBufferChannelCount;
		}

		return true;
	}
	return false;
}

@end
