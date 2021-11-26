/**
 * @file
 * VuoAvWriterObject implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "module.h"
#include "VuoAvWriterObject.h"
#include "VuoOsStatus.h"
#include <CoreVideo/CoreVideo.h>
#import <AVFoundation/AVFoundation.h>
#include <OpenGL/CGLMacro.h>
#include <sys/stat.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoAvWriterObject",
					 "dependencies" : [
						"VuoFont",
						"VuoImage",
						"VuoImageText",
						"VuoAudioSamples",
						"VuoImageRenderer",
						"VuoOsStatus",
						"AVFoundation.framework",
						"CoreMedia.framework",
						"CoreVideo.framework"
					 ]
				 });
#endif

const double TIMEBASE = 1000.;  ///< The resolution with which to measure time.  This means 1./1000 sec.

const long MIN_AUDIO_BITRATE = 64000;	///< Minimum audio bitrate used when encoding AAC.
const long MAX_AUDIO_BITRATE = 320000;	///< Maximum audio bitrate used when encoding AAC.

/**
 * Manages writing audio and video samples to a movie file.  Do not use this class directly - use VuoAvWriter.
 */
@interface VuoAvWriterObject()
@property (retain) AVAssetWriter* assetWriter;  ///< AssetWriter object.  Created and destroyed for every video file.
@property (retain) AVAssetWriterInput* videoInput;  ///< Video input.
@property (retain) AVAssetWriterInput* audioInput;  ///< Audio input.
@property (retain) AVAssetWriterInputPixelBufferAdaptor* avAdaptor;  ///< Convert from OpenGL texture to CVPixelBuffer.

@property CMFormatDescriptionRef audio_fmt_desc;  ///< Describes how the audio input samples are received.  Eg, float, channels, packing, etc.
@property long audio_sample_position;  ///< The current frame of audio being written.  Increases by sampleCount for each audioSample array fed in.

@property (retain) NSDate* startDate;  ///< The time and date that the first frame was written.
@property CMTime lastImageTimestamp;	///< Last image timestamp written to the movie file
@property CMTime lastAudioTimestamp;	///< Last audio timestamp written to the movie file

@property int originalChannelCount;  ///< How many audio channels are received in `appendAudio`.  Cannot change once writing has commenced.
@property int originalWidth;	///< The width of image this movie will write.  Cannot change once writing has commenced.
@property int originalHeight;	///< The width of image this movie will write.  Cannot change once writing has commenced.
@property bool firstFrame;  ///< Used to set the startDate in `appendImage` or `appendAudio`, which is in turn used for calculating timestamp.
@end

@implementation VuoAvWriterObject

- (BOOL) isRecording
{
	return self.assetWriter != nil;
}

- (BOOL) setupAssetWriterWithUrl:(NSURL*) fileUrl imageWidth:(int)width imageHeight:(int)height channelCount:(int)channels movieFormat:(VuoMovieFormat)format
{
	NSError *error = nil;

	self.lastImageTimestamp = kCMTimeNegativeInfinity;
	self.lastAudioTimestamp = kCMTimeNegativeInfinity;

	self.originalWidth = width;
	self.originalHeight = height;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// The replacements, AVVideoCodecType*, aren't available until macOS 10.13.
	NSString* videoEncoding = AVVideoCodecJPEG;

	if (format.imageEncoding == VuoMovieImageEncoding_H264)
		videoEncoding = AVVideoCodecH264;
	else if(format.imageEncoding == VuoMovieImageEncoding_ProRes4444)
		videoEncoding = AVVideoCodecAppleProRes4444;
	else if(format.imageEncoding == VuoMovieImageEncoding_ProRes422)
		videoEncoding = AVVideoCodecAppleProRes422;
#pragma clang diagnostic pop
	else if (format.imageEncoding == VuoMovieImageEncoding_ProRes422HQ)
	{
		if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10,15,0}])
			videoEncoding = @"apch"; // AVVideoCodecTypeAppleProRes422HQ
		else
		{
			VUserLog("Error: macOS %d.%d doesn't support ProRes 422 HQ.", (int)NSProcessInfo.processInfo.operatingSystemVersion.majorVersion, (int)NSProcessInfo.processInfo.operatingSystemVersion.minorVersion);
			return NO;
		}
	}
	else if (format.imageEncoding == VuoMovieImageEncoding_ProRes422LT)
	{
		if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10,15,0}])
			videoEncoding = @"apcs"; // AVVideoCodecTypeAppleProRes422LT
		else
		{
			VUserLog("Error: macOS %d.%d doesn't support ProRes 422 LT.", (int)NSProcessInfo.processInfo.operatingSystemVersion.majorVersion, (int)NSProcessInfo.processInfo.operatingSystemVersion.minorVersion);
			return NO;
		}
	}
	else if (format.imageEncoding == VuoMovieImageEncoding_ProRes422Proxy)
	{
		if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10,15,0}])
			videoEncoding = @"apco"; // AVVideoCodecTypeAppleProRes422Proxy
		else
		{
			VUserLog("Error: macOS %d.%d doesn't support ProRes 422 Proxy.", (int)NSProcessInfo.processInfo.operatingSystemVersion.majorVersion, (int)NSProcessInfo.processInfo.operatingSystemVersion.minorVersion);
			return NO;
		}
	}
	else if (format.imageEncoding == VuoMovieImageEncoding_HEVC)
	{
		if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10,13,0}])
			videoEncoding = @"hvc1"; // AVVideoCodecTypeHEVC
		else
		{
			VUserLog("Error: macOS %d.%d doesn't support HEVC/h.265.", (int)NSProcessInfo.processInfo.operatingSystemVersion.majorVersion, (int)NSProcessInfo.processInfo.operatingSystemVersion.minorVersion);
			return NO;
		}
	}
	else if (format.imageEncoding == VuoMovieImageEncoding_HEVCAlpha)
	{
		if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10,15,0}])
			videoEncoding = @"muxa"; // AVVideoCodecTypeHEVCWithAlpha
		else
		{
			VUserLog("Error: macOS %d.%d doesn't support HEVC/h.265 with alpha channel.", (int)NSProcessInfo.processInfo.operatingSystemVersion.majorVersion, (int)NSProcessInfo.processInfo.operatingSystemVersion.minorVersion);
			return NO;
		}
	}

	// allocate the writer object with our output file URL
	self.assetWriter = [[[AVAssetWriter alloc] initWithURL:fileUrl fileType:AVFileTypeQuickTimeMovie error:&error] autorelease];
	_assetWriter.movieFragmentInterval = CMTimeMake(TIMEBASE*10, TIMEBASE);

	if (error) {
		VUserLog("AVAssetWriter initWithURL failed with error %s", [[error localizedDescription] UTF8String]);
		return NO;
	}

	// https://developer.apple.com/library/mac/documentation/AVFoundation/Reference/AVFoundation_Constants/#//apple_ref/doc/constant_group/Video_Settings
	NSMutableDictionary *videoOutputSettings = [@{
		AVVideoCodecKey: videoEncoding,
		AVVideoWidthKey: [NSNumber numberWithInt:self.originalWidth],
		AVVideoHeightKey: [NSNumber numberWithInt:self.originalHeight]
	} mutableCopy];

	float fudge = 0.75;
	float clampedQuality = MAX(format.imageQuality, 0.01);
	float bitrate = clampedQuality * width * height * 60. * fudge;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// The replacements, AVVideoCodecType*, aren't available until macOS 10.13.
	if( [videoEncoding isEqualToString:AVVideoCodecJPEG] )
		videoOutputSettings[AVVideoCompressionPropertiesKey] = @{
			AVVideoQualityKey: @(clampedQuality),
		};
	else if ([videoEncoding isEqualToString:AVVideoCodecH264]
		|| [videoEncoding isEqualToString:@"hvc1" /*AVVideoCodecTypeHEVC*/])
		videoOutputSettings[AVVideoCompressionPropertiesKey] = @{
			AVVideoAverageBitRateKey: @(bitrate),
		};
#pragma clang diagnostic pop
	else if ([videoEncoding isEqualToString:@"muxa" /*AVVideoCodecTypeHEVCWithAlpha*/])
	{
		videoOutputSettings[AVVideoCompressionPropertiesKey] = @{
			AVVideoAverageBitRateKey: @(bitrate),
			@"TargetQualityForAlpha" /*kVTCompressionPropertyKey_TargetQualityForAlpha*/: @(clampedQuality),
			@"AlphaChannelMode" /*kVTCompressionPropertyKey_AlphaChannelMode*/: @"PremultipliedAlpha",  // kVTAlphaChannelMode_PremultipliedAlpha
		};
	}

	if (videoOutputSettings[AVVideoCompressionPropertiesKey][AVVideoAverageBitRateKey])
		VUserLog("Asking AV Foundation to encode with average bitrate %0.2g Mbit/sec (%g * %d * %d * 60 * %g).", bitrate / 1024.f / 1024.f, clampedQuality, width, height, fudge);

	self.videoInput = [[[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:videoOutputSettings] autorelease];
	[videoOutputSettings release];
	[self.videoInput setExpectsMediaDataInRealTime:YES];

	NSDictionary *pa = @{
			(NSString *)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithInt:kCVPixelFormatType_32BGRA],
			(NSString *)kCVPixelBufferWidthKey: [NSNumber numberWithInt:self.originalWidth],
			(NSString *)kCVPixelBufferHeightKey: [NSNumber numberWithInt:self.originalHeight],
		};

	self.avAdaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:self.videoInput sourcePixelBufferAttributes:pa];

	if ([self.assetWriter canAddInput:self.videoInput])
	{
		[self.assetWriter addInput:self.videoInput];
	}
	else
	{
		VUserLog("Failed adding a video input to the AVWriter.");
		self.assetWriter = nil;
		self.videoInput	= nil;
		self.avAdaptor = nil;

		return NO;
	}

	self.originalChannelCount = channels;

	/// AUDIO AUDIO AUDIO https://stackoverflow.com/questions/12187124/writing-video-generated-audio-to-avassetwriterinput-audio-stuttering
	if(channels > 0)
	{
		AudioStreamBasicDescription audioFormat;
		bzero(&audioFormat, sizeof(audioFormat));

		audioFormat.mSampleRate = VuoAudioSamples_sampleRate;
		audioFormat.mFormatID   = kAudioFormatLinearPCM;
		audioFormat.mFramesPerPacket = 1;
		audioFormat.mChannelsPerFrame = channels;
		int bytes_per_sample = sizeof(float);
		audioFormat.mFormatFlags = kAudioFormatFlagIsFloat;
		audioFormat.mBitsPerChannel = bytes_per_sample * 8;
		audioFormat.mBytesPerPacket = bytes_per_sample * channels;
		audioFormat.mBytesPerFrame = bytes_per_sample * channels;

		CMFormatDescriptionRef fmt;
		CMAudioFormatDescriptionCreate(kCFAllocatorDefault,
									   &audioFormat,
									   0,
									   NULL,
									   0,
									   NULL,
									   NULL,
									   &fmt
									   );
		self.audio_fmt_desc = fmt;

		size_t aclSize = sizeof(AudioChannelLayout);
		AudioChannelLayout *acl = malloc(aclSize);
		bzero(acl, aclSize);
		if (channels == 1)
			acl->mChannelLayoutTag = kAudioChannelLayoutTag_Mono;
		else if (channels == 2)
			acl->mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
		else
		{
			aclSize = sizeof(AudioChannelLayout) + sizeof(AudioChannelDescription) * (channels - 1);
			acl = realloc(acl, aclSize);
			bzero(acl, aclSize);
			acl->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
			acl->mNumberChannelDescriptions = channels;
			for (int i = 0; i < channels; ++i)
				acl->mChannelDescriptions[i].mChannelLabel = kAudioChannelLabel_Discrete_0 + i;
		}

		int audioEncoding = kAudioFormatLinearPCM;
		NSDictionary* audioOutputSettings;

		if(format.audioEncoding == VuoAudioEncoding_LinearPCM)
		{
			audioEncoding = kAudioFormatLinearPCM;
			audioOutputSettings = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithInt: audioEncoding ], AVFormatIDKey,
										[NSNumber numberWithInt: channels ], AVNumberOfChannelsKey,
										[NSNumber numberWithFloat: VuoAudioSamples_sampleRate], AVSampleRateKey,
										[NSNumber numberWithInt: channels],AVNumberOfChannelsKey,
										[NSNumber numberWithInt:16], AVLinearPCMBitDepthKey,
										[NSNumber numberWithBool:NO], AVLinearPCMIsBigEndianKey,
										[NSNumber numberWithBool:NO], AVLinearPCMIsFloatKey,
										[NSNumber numberWithBool:NO], AVLinearPCMIsNonInterleaved,
										[NSData dataWithBytes:acl length:aclSize], AVChannelLayoutKey,
										nil];
		}
		else
		{
			audioEncoding = kAudioFormatMPEG4AAC;
			long audioBitrate = (long)(MIN_AUDIO_BITRATE + ((float)(MAX_AUDIO_BITRATE-MIN_AUDIO_BITRATE) * format.audioQuality));

			audioOutputSettings = [NSDictionary dictionaryWithObjectsAndKeys:
										[ NSNumber numberWithInt: audioEncoding ], AVFormatIDKey,
										[ NSNumber numberWithInt: channels ], AVNumberOfChannelsKey,
										[ NSNumber numberWithFloat: VuoAudioSamples_sampleRate], AVSampleRateKey,
										[ NSNumber numberWithInt:audioBitrate], AVEncoderBitRateKey,
										[NSData dataWithBytes:acl length:aclSize], AVChannelLayoutKey,
										nil];
		}
		free(acl);

		self.audioInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio outputSettings:audioOutputSettings];
		self.audioInput.expectsMediaDataInRealTime=YES;
		self.audio_sample_position = 0;

		if([self.assetWriter canAddInput:self.audioInput])
		{
			[self.assetWriter addInput:self.audioInput];
		}
		else
		{
			VUserLog("Could not add audio input.");
			self.audioInput = nil;
		}
	}

	// initiates a sample-writing at time 0
	[self.assetWriter startWriting];
	[self.assetWriter startSessionAtSourceTime:kCMTimeZero];

	self.firstFrame = true;

	return YES;
}

- (void) appendImage:(VuoImage)image presentationTime:(double)timestamp blockIfNotReady:(BOOL)blockIfNotReady
{
	if(image->pixelsWide != self.originalWidth || image->pixelsHigh != self.originalHeight)
	{
		VUserLog("Error: Can't append image because it is not the same dimensions as the the current movie.");
		return;
	}

	if(!self.videoInput.readyForMoreMediaData)
	{
		if (blockIfNotReady)
		{
			VUserLog("Warning: The AVFoundation asset writer isn't keeping up; waiting for it to catch up before writing this video frame.");
			while (!self.videoInput.readyForMoreMediaData)
				usleep(USEC_PER_SEC/10);
		}
		else
		{
			VUserLog("Error: The AVFoundation asset writer isn't keeping up; dropping this video frame.");
			return;
		}
	}

	CVPixelBufferRef pb = nil;

	const unsigned char *buf = VuoImage_getBuffer(image, GL_BGRA);

	CVReturn ret = CVPixelBufferPoolCreatePixelBuffer(nil, [self.avAdaptor pixelBufferPool], &pb);

	if(ret != kCVReturnSuccess)
	{
		VUserLog("Error: Couldn't get PixelBuffer from pool. %d", ret);
		return;
	}

	ret = CVPixelBufferLockBaseAddress(pb, 0);

	if(ret != kCVReturnSuccess)
	{
		VUserLog("Error: Couldn't lock PixelBuffer base address");
		return;
	}

	unsigned char *bytes = (unsigned char*)CVPixelBufferGetBaseAddress(pb);

	if(!bytes)
	{
		VUserLog("Error: Couldn't get the pixel buffer base address");
		return;
	}

	unsigned int bytesPerRow = CVPixelBufferGetBytesPerRow(pb);

	for(unsigned long y = 0; y < self.originalHeight; y++)
		memcpy(bytes + bytesPerRow * (self.originalHeight - y - 1), buf + self.originalWidth * y * 4, self.originalWidth * 4);

	ret = CVPixelBufferUnlockBaseAddress(pb, 0);

	if(ret != kCVReturnSuccess)
	{
		VUserLog("Error: Couldn't unlock pixelbuffer base address. %d", ret);
		return;
	}

	CMTime presentationTime = CMTimeMakeWithSeconds(timestamp, TIMEBASE);

	while (CMTimeCompare(presentationTime, self.lastImageTimestamp) <= 0)
		presentationTime.value++;

	self.lastImageTimestamp = presentationTime;

//	VLog("video time: %lld %f  pts: %f  ts=%g", presentationTime.value, [[NSDate date] timeIntervalSinceDate:self.startDate], CMTimeGetSeconds(presentationTime),timestamp);

	if (![self.avAdaptor appendPixelBuffer:pb withPresentationTime:presentationTime])
	{
		VUserLog("Couldn't write frame %lld (%fs): %s", presentationTime.value, CMTimeGetSeconds(presentationTime), [[_assetWriter.error description] UTF8String]);
	}

	if(pb)
		CVPixelBufferRelease(pb);
}

- (void) appendAudio:(VuoList_VuoAudioSamples) samples presentationTime:(VuoReal)timestamp blockIfNotReady:(BOOL)blockIfNotReady
{
	long sampleCount = VuoAudioSamples_bufferSize;
	long channelCount = VuoListGetCount_VuoAudioSamples(samples);
	if (channelCount == 0)
		return;

	if(channelCount != self.originalChannelCount)
	{
		if (self.originalChannelCount == -1)
			VUserLog("Error: Attempting to write %lu audio channels to a silent movie.", channelCount);
		else
			VUserLog("Error: Attempting to write %lu audio channels to a movie with %d channels.", channelCount, self.originalChannelCount);
		return;
	}

	if ( !self.audioInput.readyForMoreMediaData) {
		if (blockIfNotReady)
		{
			VUserLog("Warning: The AVFoundation asset writer isn't keeping up; waiting for it to catch up before writing this audio frame.");
			while (!self.audioInput.readyForMoreMediaData)
				usleep(USEC_PER_SEC/10);
		}
		else
		{
			VUserLog("Error: The AVFoundation asset writer isn't keeping up; dropping this audio frame.");
			return;
		}
	}

	OSStatus status;
	CMBlockBufferRef bbuf = NULL;
	CMSampleBufferRef sbuf = NULL;

	size_t buflen = sampleCount * channelCount * sizeof(float);

	float* interleaved = (float*)malloc(sizeof(float) * sampleCount * channelCount);
	VuoDefer(^{ free(interleaved); });

	for(int n = 0; n < channelCount; n++)
	{
		VuoAudioSamples s = VuoListGetValue_VuoAudioSamples(samples, n+1);
		double *channel = s.samples;
		if (!channel)
		{
			VUserLog("Error: Attempting to write a NULL audio sample buffer.  Skipping this audio frame.");
			return;
		}
		if (s.sampleCount != VuoAudioSamples_bufferSize)
		{
			VUserLog("Error: Attempting to write an audio sample buffer that has %lld samples, but expected %lld.  Skipping this audio frame.", s.sampleCount, VuoAudioSamples_bufferSize);
			return;
		}

		for(int i = 0; i < sampleCount; i++)
		{
			interleaved[i*channelCount + n] = channel[i];
		}
	}

	// Create sample buffer for adding to the audio input.
	CMBlockBufferRef tmp;
	status = CMBlockBufferCreateWithMemoryBlock( kCFAllocatorDefault,
												 interleaved,
												 buflen,
												 kCFAllocatorNull,
												 NULL,
												 0,
												 buflen,
												 0,
												 &tmp);
	if (status != noErr) {
		char *s = VuoOsStatus_getText(status);
		VUserLog("CMBlockBufferCreateWithMemoryBlock error: %s", s);
		free(s);
		return;
	}

	status = CMBlockBufferCreateContiguous(kCFAllocatorDefault, tmp, kCFAllocatorDefault, NULL, 0,
										   buflen, kCMBlockBufferAlwaysCopyDataFlag, &bbuf);

	CFRelease(tmp);

	if (status != noErr) {
		char *s = VuoOsStatus_getText(status);
		VUserLog("CMBlockBufferCreateContiguous error: %s", s);
		free(s);
		return;
	}

	CMTime presentationTime = CMTimeMakeWithSeconds(timestamp, TIMEBASE);

	// if(self.firstFrame)
	// {
	// 	self.firstFrame = false;
	// 	self.startDate = [NSDate date];
	// 	timestamp = CMTimeMake(0, AUDIO_TIMEBASE);
	// }
	// else
	// {
	// 	timestamp = CMTimeMake([[NSDate date] timeIntervalSinceDate:self.startDate] * AUDIO_TIMEBASE, AUDIO_TIMEBASE);
	// }

	// CMTime timestamp = CMTimeMake(self.audio_sample_position, VuoAudioSamples_sampleRate);
	// self.audio_sample_position += sampleCount;

	while (CMTimeCompare(presentationTime, self.lastAudioTimestamp) <= 0)
		presentationTime.value++;

	self.lastAudioTimestamp = presentationTime;

//	VLog("audio time: %lld %f  pts: %f  ts=%g", presentationTime.value, [[NSDate date] timeIntervalSinceDate:self.startDate], CMTimeGetSeconds(presentationTime),timestamp);

	status = CMAudioSampleBufferCreateWithPacketDescriptions(	kCFAllocatorDefault,
																bbuf,
																TRUE,
																0,
																NULL,
																self.audio_fmt_desc,
																VuoAudioSamples_bufferSize,
																presentationTime,
																NULL, &sbuf);

	if (status != noErr) {
		VUserLog("CMSampleBufferCreate error");
		return;
	}

	if ( ![self.audioInput appendSampleBuffer:sbuf] )
	{
		VUserLog("AppendSampleBuffer error");
	}

	CFRelease(bbuf);
	CFRelease(sbuf);
	bbuf = nil;
	sbuf = nil;
}

- (void) finalizeRecording
{
	if( self.assetWriter )
	{
		[self.videoInput markAsFinished];

		if(self.audioInput != nil)
			[self.audioInput markAsFinished];

		dispatch_semaphore_t finishedWriting = dispatch_semaphore_create(0);
		[_assetWriter finishWritingWithCompletionHandler:^{
			dispatch_semaphore_signal(finishedWriting);
		}];
		dispatch_semaphore_wait(finishedWriting, DISPATCH_TIME_FOREVER);
		dispatch_release(finishedWriting);

		if (_assetWriter.status != AVAssetWriterStatusCompleted)
			VUserLog("Error: %s", [[_assetWriter.error localizedDescription] UTF8String]);

		self.assetWriter = nil;
		self.videoInput  = nil;
		self.audioInput = nil;
	}
}

- (void) dealloc
{
	if(self.assetWriter)
		[self finalizeRecording];

	[super dealloc];
}

@end
