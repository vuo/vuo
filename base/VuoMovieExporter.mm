/**
 * @file
 * VuoMovieExporter implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#import "VuoMovieExporter.hh"

#include <sys/stat.h>

#import <stdexcept>

#define NS_RETURNS_INNER_POINTER
#import <OpenGL/CGLMacro.h>
#import <AVFoundation/AVFoundation.h>

#import "VuoLog.h"
#import "VuoRunnerCocoa.h"
#import "VuoFileUtilities.hh"
#import "VuoImageResize.h"
extern "C" {
#import "VuoGlContext.h"
#import "VuoImage.h"
#import "VuoImageWatermark.h"
#import "VuoText.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop
}


/**
 * Returns true if ProRes4444 and ProRes422 are available on this system.
 *
 * On Mac OS 10.9 and later, they're always available.
 *
 * On Mac OS 10.8 and 10.7, they require Final Cut Pro to be installed.
 */
bool VuoMovieExporterParameters::isProResAvailable(void)
{
	SInt32 macMinorVersion;
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
	if (macMinorVersion >= 9)
		return true;

	struct stat buf;
	if (stat("/Library/QuickTime/AppleProResCodec.component", &buf) == 0)
		return true;
	else
		return false;
}

/**
 * Initializes movie exporter parameters.
 */
VuoMovieExporterParameters::VuoMovieExporterParameters(int width, int height, double time, double duration, double framerate, int spatialSupersample, int temporalSupersample, float shutterAngle, enum ImageFormat imageFormat, double quality, bool watermark)
{
	this->width = width;
	this->height = height;
	this->time = time;
	this->duration = duration;
	this->framerate = framerate;
	this->spatialSupersample = spatialSupersample;
	this->temporalSupersample = temporalSupersample;
	this->shutterAngle = shutterAngle;
	this->imageFormat = imageFormat;
	this->quality = quality;
	this->watermark = watermark;
}

/**
 * Prints a description of the parameters to STDOUT.
 */
void VuoMovieExporterParameters::print(void)
{
	printf("Suggested output size: %dx%d\n", width, height);
	printf("Starting at %g second%s and rendering %g frame%s per second for %g second%s.\n",
		   time, (time == 1 ? "" : "s"),
		   framerate, (framerate == 1 ? "" : "s"),
		   duration, (duration == 1 ? "" : "s"));
	if (spatialSupersample > 1)
		printf("Each frame will be rendered at %dx resolution, then scaled down to the output size.\n", spatialSupersample);
	if (temporalSupersample > 1)
		printf("Each frame will be rendered %d time%s (shutter angle %g°), then averaged.\n", temporalSupersample, (temporalSupersample == 1 ? "" : "s"), shutterAngle);

	std::string imageFormatString;
	if (imageFormat == H264)
		imageFormatString = "H.264";
	else if (imageFormat == JPEG)
		imageFormatString = "JPEG";
	else if (imageFormat == ProRes4444)
		imageFormatString = "ProRes 4444";
	else if (imageFormat == ProRes422)
		imageFormatString = "ProRes 422";
	printf("Frames will be saved as %s with quality = %g.\n", imageFormatString.c_str(), quality);
//	if (imageFormat == H264)
//	{
//		printf("Estimated bitrate: %g megabits/second\n", getEstimatedBitrate() / 1024 / 1024);
//		printf("Estimated total size: %ld megabytes\n", getEstimatedSize() / 1024 / 1024);
//	}

	if (inputs.size())
	{
		printf("Input port values:\n");
		for (std::map<std::string, json_object *>::const_iterator i = inputs.begin(); i != inputs.end(); ++i)
			printf("\t%s = %s\n", i->first.c_str(), json_object_to_json_string(i->second));
	}
}

/**
 * Returns the estimated average bitrate, in bits per second.
 *
 * Only valid for `H264`.
 */
double VuoMovieExporterParameters::getEstimatedBitrate(void)
{
	// Try to guess reasonable bitrates for quality values between 0 and 1.
	float fudge = 0.75;
	return MAX(0.01, quality) * width * height * framerate * fudge;
}

/**
 * Returns the estimated file size, in bytes.
 *
 * Only valid for `H264`.
 */
unsigned long VuoMovieExporterParameters::getEstimatedSize(void)
{
	return getEstimatedBitrate() * duration / 8;
}

/**
 * Initializes a movie exporter instance with a composition file.
 */
VuoMovieExporter::VuoMovieExporter(std::string compositionFile, std::string outputMovieFile, VuoMovieExporterParameters parameters)
{
	NSURL *compositionUrl = [NSURL fileURLWithPath:[NSString stringWithUTF8String:compositionFile.c_str()]];
	NSString *compositionString = [NSString stringWithContentsOfURL:compositionUrl encoding:NSUTF8StringEncoding error:NULL];

	std::string dir, file, ext;
	VuoFileUtilities::splitPath(compositionFile, dir, file, ext);

	init(compositionString, file, dir, outputMovieFile, parameters);
}

/**
 * Initializes a movie exporter instance with a composition in a string.
 */
VuoMovieExporter::VuoMovieExporter(std::string compositionString, std::string name, std::string sourcePath, std::string outputMovieFile, VuoMovieExporterParameters parameters)
{
	init([NSString stringWithUTF8String:compositionString.c_str()], name, sourcePath, outputMovieFile, parameters);
}

/**
 * Returns the greatest common divisor of a and b.
 */
static int gcd(int a, int b)
{
	while (b > 0)
	{
		int rem = a % b;
		a = b;
		b = rem;
	}
	return a;
}

/**
 * Helper for the constructors.
 *
 * @throw std::runtime_error Couldn't build the composition or create the movie file.
 */
void VuoMovieExporter::init(NSString *compositionString, std::string name, std::string sourcePath, std::string outputMovieFile, VuoMovieExporterParameters parameters)
{
	if (parameters.imageFormat == VuoMovieExporterParameters::H264)
	{
		// H.264 Level 5.1 (highest available as of 2015.07.15) only supports up to 36,864 macroblocks.
		// Instead of letting AVFoundation throw a nondescript "-6661 invalid argument" error,
		// try to catch it ourselves and provide a more helpful message.
		// https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC#Levels
		int widthRoundedToMacroblock  = ceil((double)parameters.width /16.)*16.;
		int heightRoundedToMacroblock = ceil((double)parameters.height/16.)*16.;
		if (widthRoundedToMacroblock * heightRoundedToMacroblock > 36864*(16*16))
			throw std::runtime_error("The H.264 codec only allows up to 9,437,184 pixels per frame. Try another image format, or a lower resolution.");
	}

	this->parameters = parameters;

	// Given the framerate, find an appropriate timebase and frame multiplier.
	{
		int maxTimeResolution = 1000;
		int numerator   = maxTimeResolution * 1;
		int denominator = maxTimeResolution * parameters.framerate;
		int divisor = gcd(numerator, denominator);
		frameMultiplier = numerator   / divisor;
		timebase        = denominator / divisor;
	}

	frameCount = parameters.duration * parameters.framerate;
	framesExportedSoFar = 0;
	firstFrame = true;

	assetWriter = NULL;
	assetWriterInput = NULL;
	assetWriterInputPixelBufferAdaptor = NULL;

	outputMovieUrl = [[NSURL alloc] initFileURLWithPath:[NSString stringWithUTF8String:outputMovieFile.c_str()]];

	// Set up resize shader.
	{
		resizeShader = VuoImageResize_makeShader();
		VuoRetain(resizeShader);

		// Needs its own context since VuoImageRenderer changes the glViewport.
		resizeContext = VuoGlContext_use();

		resizeImageRenderer = VuoImageRenderer_make(resizeContext);
		VuoRetain(resizeImageRenderer);
	}

	blender = VuoImageBlend_make();
	VuoRetain(blender);


	// AVAssetWriter fails if the destination path already exists.
	// Since the user might choose to replace the existing file,
	// delete it if it already exists.
	if ([[NSFileManager defaultManager] fileExistsAtPath:[outputMovieUrl path]])
	{
		NSError *error;
		if (![[NSFileManager defaultManager] removeItemAtURL:outputMovieUrl error:&error])
			throw std::runtime_error("movie file already exists, and can't be deleted");
	}

	if (![VuoImageGenerator canOpenCompositionString:compositionString])
		throw std::runtime_error("composition file doesn't exist, or doesn't adhere to the Image Generator protocol");

	NSString *sourcePathNSString = [[NSString alloc] initWithUTF8String:sourcePath.c_str()];
	NSString *nameNSString       = [[NSString alloc] initWithUTF8String:name.c_str()];
	generator = [[VuoImageGenerator alloc] initWithCompositionString:compositionString name:nameNSString sourcePath:sourcePathNSString];
	if (!generator)
		throw std::runtime_error("couldn't initialize VuoImageGenerator");

	for (std::map<std::string, json_object *>::const_iterator i = parameters.inputs.begin(); i != parameters.inputs.end(); ++i)
		if (![generator setJSONValue:i->second forInputPort:[NSString stringWithUTF8String:i->first.c_str()]])
		{
			[generator release];
			throw std::runtime_error(std::string("failed to set value for port '") + i->first + "'");
		}

	[sourcePathNSString release];
}

/**
 * Finalizes the movie.
 *
 * @throw std::runtime_error Couldn't finalize the movie.
 */
VuoMovieExporter::~VuoMovieExporter()
{
	if (assetWriter)
	@try
	{
		// Fancy dance to ensure -finishWriting isn't called on the main thread, since that needlessly throws a warning.
		dispatch_semaphore_t finishedWriting = dispatch_semaphore_create(0);
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			/// @todo Replace with -finishWritingWithCompletionHandler: when we drop Mac OS 10.8 support.
			if (![assetWriter finishWriting])
				throw std::runtime_error([[assetWriter.error localizedDescription] UTF8String]);
			dispatch_semaphore_signal(finishedWriting);
		});
		dispatch_semaphore_wait(finishedWriting, DISPATCH_TIME_FOREVER);
		dispatch_release(finishedWriting);

		[assetWriter release];
		// Ownership of assetWriterInput and assetWriterInputPixelBufferAdaptor has transferred to assetWriter,
		// so no need to release them.
	}
	@catch (NSException *exception)
	{
		throw std::runtime_error([[exception description] UTF8String]);
	}

	[outputMovieUrl release];
	[generator release];

	VuoRelease(resizeImageRenderer);
	VuoGlContext_disuse(resizeContext);
	VuoRelease(resizeShader);

	VuoRelease(blender);
}

/**
 * Returns the total number of frames to export.
 */
unsigned int VuoMovieExporter::getTotalFrameCount(void)
{
	return frameCount;
}

#if BASE_PREMIUM_AVAILABLE
#include "premium/VuoMovieExporterPremium.h"
#else
VuoImage VuoMovieExporter::exportNextFramePremium(void)
{
	throw std::runtime_error("this version of Vuo was not built with support for spatial and temporal supersampling");
}
#endif

/**
 * Exports 1 frame.
 *
 * Returns false if exporting failed, or if enough frames have already been exported.
 *
 * @throw std::runtime_error Couldn't convert the frame for export.
 */
bool VuoMovieExporter::exportNextFrame(void)
{
	if (framesExportedSoFar >= frameCount)
		return false;


	[generator setValue:(id)kCFBooleanTrue      forInputPort:@"offlineRender"];
	[generator setValue:@(parameters.framerate) forInputPort:@"offlineFramerate"];


	// Render the image.
	VuoImage image;
	const unsigned char *pixels;
	unsigned int pixelsWide;
	unsigned int pixelsHigh;
	{
		if (parameters.spatialSupersample > 1 || parameters.temporalSupersample > 1)
		{
			image = exportNextFramePremium();
			if (!image)
				goto done;
		}
		else
		{
			image = [generator generateVuoImageWithSuggestedPixelsWide: parameters.width
															pixelsHigh: parameters.height
																atTime: (parameters.time + framesExportedSoFar / parameters.framerate)];
			if (!image)
				goto done;

			VuoRetain(image);

			if (image->pixelsWide != parameters.width || image->pixelsHigh != parameters.height)
			{
				VuoImage resizedImage = VuoImageResize_resize(image, resizeShader, resizeImageRenderer, VuoSizingMode_Fit, parameters.width, parameters.height);
				if (!resizedImage)
				{
					VuoRelease(image);
					throw std::runtime_error("failed to resize image");
				}
				VuoRetain(resizedImage);
				VuoRelease(image);
				image = resizedImage;
			}
//			VLog("generated %ldx%ld",image->pixelsWide,image->pixelsHigh);
		}

		if (this->parameters.watermark)
		{
			VuoImage watermarkedImage = VuoImage_watermark(image);
			VuoRelease(image);
			VuoRetain(watermarkedImage);
			image = watermarkedImage;
		}

		pixels = VuoImage_getBuffer(image, GL_BGRA);
		pixelsWide = image->pixelsWide;
		pixelsHigh = image->pixelsHigh;
	}


	// Append it to the movie.
	@try
	{
		if (firstFrame)
		{
			NSError *e = nil;
			assetWriter = [[AVAssetWriter alloc] initWithURL:outputMovieUrl fileType:AVFileTypeQuickTimeMovie error:&e];

			NSString *codec = nil;
			if (parameters.imageFormat == VuoMovieExporterParameters::H264)
				codec = AVVideoCodecH264;
			else if (parameters.imageFormat == VuoMovieExporterParameters::JPEG)
				codec = AVVideoCodecJPEG;
			else if (parameters.imageFormat == VuoMovieExporterParameters::ProRes4444)
			{
				if (!VuoMovieExporterParameters::isProResAvailable())
				{
					[assetWriter release];
					assetWriter = nil;
					throw std::runtime_error("The ProRes codec is only available on systems with Final Cut Pro installed.");
				}
				codec = AVVideoCodecAppleProRes4444;
			}
			else if (parameters.imageFormat == VuoMovieExporterParameters::ProRes422)
			{
				if (!VuoMovieExporterParameters::isProResAvailable())
				{
					[assetWriter release];
					assetWriter = nil;
					throw std::runtime_error("The ProRes codec is only available on systems with Final Cut Pro installed.");
				}
				codec = AVVideoCodecAppleProRes422;
			}

			NSMutableDictionary *videoSettings = [@{
				AVVideoCodecKey: codec,
				AVVideoWidthKey:  [NSNumber numberWithInt:pixelsWide],
				AVVideoHeightKey: [NSNumber numberWithInt:pixelsHigh]
			} mutableCopy];

			if (parameters.imageFormat == VuoMovieExporterParameters::H264)
				[videoSettings setObject:@{AVVideoAverageBitRateKey: [NSNumber numberWithDouble:parameters.getEstimatedBitrate()]} forKey:AVVideoCompressionPropertiesKey];
			else if (parameters.imageFormat == VuoMovieExporterParameters::JPEG)
				[videoSettings setObject:@{AVVideoQualityKey:[NSNumber numberWithDouble:MAX(0.01, parameters.quality)]} forKey:AVVideoCompressionPropertiesKey];

			assetWriterInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
			[videoSettings release];

			NSDictionary *pa = @{
				(NSString *)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithInt:kCVPixelFormatType_32BGRA],
				(NSString *)kCVPixelBufferWidthKey:  [NSNumber numberWithInt:pixelsWide],
				(NSString *)kCVPixelBufferHeightKey: [NSNumber numberWithInt:pixelsHigh],
			};

			assetWriterInputPixelBufferAdaptor = [AVAssetWriterInputPixelBufferAdaptor
				assetWriterInputPixelBufferAdaptorWithAssetWriterInput:assetWriterInput
				sourcePixelBufferAttributes:pa];

			if (![assetWriter canAddInput:assetWriterInput])
				throw std::runtime_error(std::string("error adding AVAssetWriterInput: ") + [[assetWriter.error description] UTF8String]);
			[assetWriter addInput:assetWriterInput];

			if (![assetWriter startWriting])
				throw std::runtime_error(std::string("error starting writing: ") + [[assetWriter.error description] UTF8String]);

			[assetWriter startSessionAtSourceTime:CMTimeMake(0, timebase)];

			this->firstFrame = false;
		}

		CVPixelBufferRef pb;
		CVReturn ret = CVPixelBufferPoolCreatePixelBuffer(NULL, [assetWriterInputPixelBufferAdaptor pixelBufferPool], &pb);
		if (ret != kCVReturnSuccess)
			throw std::runtime_error(VuoText_format("couldn't get buffer from pool: %d", ret));

		ret = CVPixelBufferLockBaseAddress(pb, 0);
		if (ret != kCVReturnSuccess)
			throw std::runtime_error(VuoText_format("error locking buffer: %d", ret));

		unsigned char *bytes = (unsigned char *)CVPixelBufferGetBaseAddress(pb);
		if (!bytes)
			throw std::runtime_error("error getting buffer base address");

		// Flip the image data (OpenGL returns flipped data, but CVPixelBufferRef assumes it is not flipped),
		// while copying it into `bytes`.
		unsigned int bytesPerRow = CVPixelBufferGetBytesPerRow(pb);
		for (unsigned long y = 0; y < pixelsHigh; ++y)
			memcpy(bytes + bytesPerRow * (pixelsHigh - y - 1), pixels + pixelsWide * y * 4, pixelsWide * 4);
		VuoRelease(image);

		ret = CVPixelBufferUnlockBaseAddress(pb, 0);
		if (ret != kCVReturnSuccess)
			throw std::runtime_error(VuoText_format("error unlocking buffer: %d", ret));

		while (!assetWriterInput.readyForMoreMediaData)
			usleep(USEC_PER_SEC / 10);

		CMTime time = CMTimeMake(framesExportedSoFar * frameMultiplier, timebase);
		if (![assetWriterInputPixelBufferAdaptor appendPixelBuffer:pb withPresentationTime:time])
			throw std::runtime_error("error appending buffer");

		CVPixelBufferRelease(pb);
	}
	@catch (NSException *exception)
	{
		throw std::runtime_error([[exception description] UTF8String]);
	}

done:
	++framesExportedSoFar;
	return framesExportedSoFar < frameCount;
}
