/**
 * @file
 * VuoWindowRecorder implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoWindowRecorder.h"

#include "VuoGraphicsWindow.h"
#include "VuoGraphicsView.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include <AVFoundation/AVFoundation.h>

#include "VuoGlPool.h"
#include "VuoImageResize.h"
#include "VuoImageWatermark.h"

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowRecorder",
					 "dependencies" : [
						 "AVFoundation.framework",
						 "CoreMedia.framework",
						 "OpenGL.framework",
						 "VuoFont",
						 "VuoGlPool",
						 "VuoGraphicsWindow",
						 "VuoGraphicsView",
						 "VuoImage",
						 "VuoImageRenderer",
						 "VuoImageResize",
						 "VuoImageText",
						 "VuoShader"
					 ]
				 });
#endif


#define TIMEBASE 60	///< The movie's time resolution, in 1/seconds.


/**
 * Private variables for @ref VuoWindowRecorder.
 */
@interface VuoWindowRecorder ()
@property dispatch_queue_t queue;	///< Serializes all AVFoundation calls for this window.
@property bool first;				///< Is this the first frame?
@property bool stopping;			///< Has the caller requested that we stop recording?
@property double startTime;			///< Time (in seconds since 1970) recording started.
@property CMTime priorFrameTime;	///< Presentation time (in timebase-seconds since recording started) of the prior frame.
@property int originalWidth;		///< GL Viewport width when recording started.
@property int originalHeight;		///< GL Viewport height when recording started.
@property int priorWidth;			///< GL Viewport width during the last captured frame.
@property int priorHeight;			///< GL Viewport height during the last captured frame.
@property int frameCount;			///< Total number of frames recorded.
@property double totalSyncTime;		///< Total time spent blocking rendering.
@property double totalAsyncTime;	///< Total time spent on asyncrhonous tasks (transferring the buffer and submitting to the encoder).
@property (retain) AVAssetWriter *assetWriter;	///< AVAssetWriter
@property (retain) AVAssetWriterInput *assetWriterInput;	///< AVAssetWriterInput
@property (retain) AVAssetWriterInputPixelBufferAdaptor *assetWriterInputPixelBufferAdaptor;	///< AVAssetWriterInputPixelBufferAdaptor
@property (assign) VuoImageResize resize;	///< Shader for resizing images to fit the output movie size.
@end


@implementation VuoWindowRecorder

/**
 * Creates a record manager.
 *
 * @threadAny
 */
- (instancetype)initWithWindow:(VuoGraphicsWindow *)window url:(NSURL *)url
{
	if (self = [super init])
	{
		_queue = dispatch_queue_create("org.vuo.VuoWindowRecorder", NULL);

		_first = true;
		_priorFrameTime = CMTimeMake(-1, TIMEBASE);

		VuoGraphicsView *gv = window.contentView;
		NSRect frameInPoints = gv.frame;
		NSRect frameInPixels = [window convertRectToBacking:frameInPoints];
		_originalWidth  = _priorWidth  = frameInPixels.size.width;
		_originalHeight = _priorHeight = frameInPixels.size.height;


		_resize = VuoImageResize_make();
		VuoRetain(_resize);


		NSError *e = nil;
		self.assetWriter = [AVAssetWriter assetWriterWithURL:url fileType:AVFileTypeQuickTimeMovie error:&e];
		_assetWriter.movieFragmentInterval = CMTimeMake(TIMEBASE*10, TIMEBASE);

		NSDictionary *videoSettings = @{
			AVVideoCodecKey: AVVideoCodecH264,
			AVVideoWidthKey: [NSNumber numberWithInt:_originalWidth],
			AVVideoHeightKey: [NSNumber numberWithInt:_originalHeight]
		};


		self.assetWriterInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
		_assetWriterInput.expectsMediaDataInRealTime = YES;



		NSDictionary *pa = @{
			(NSString *)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithInt:kCVPixelFormatType_32BGRA],
			(NSString *)kCVPixelBufferWidthKey: [NSNumber numberWithInt:_originalWidth],
			(NSString *)kCVPixelBufferHeightKey: [NSNumber numberWithInt:_originalHeight],
		};

		self.assetWriterInputPixelBufferAdaptor = [AVAssetWriterInputPixelBufferAdaptor
				assetWriterInputPixelBufferAdaptorWithAssetWriterInput:_assetWriterInput
				sourcePixelBufferAttributes:pa];

		if (![_assetWriter canAddInput:_assetWriterInput])
		{
			VUserLog("Error adding AVAssetWriterInput: %s", [[_assetWriter.error description] UTF8String]);
			return nil;
		}
		[_assetWriter addInput:_assetWriterInput];

		if (![_assetWriter startWriting])
		{
			VUserLog("Error starting writing: %s", [[_assetWriter.error description] UTF8String]);
			return nil;
		}
		[_assetWriter startSessionAtSourceTime:CMTimeMake(0, TIMEBASE)];


		// Save the current image (to ensure the movie has a frame, even if the composition is stationary).
		[self saveImage:gv.ioSurface];
	}

	return self;
}


/**
 * Appends sourceBytes to the movie.
 */
- (void)appendBuffer:(const unsigned char *)sourceBytes width:(unsigned long)width height:(unsigned long)height
{
	double captureTime = VuoLogGetTime() - _startTime;

	CMTime time;
	if (_first)
	{
		time = CMTimeMake(0, TIMEBASE);
		_first = false;
		_startTime = VuoLogGetTime();
	}
	else
		time = CMTimeMake(captureTime * TIMEBASE, TIMEBASE);

	if (_stopping)
		return;

	if (!_assetWriterInput.readyForMoreMediaData)
	{
		VUserLog("Warning: AVFoundation video encoder isn't keeping up. Dropping this frame.");
		return;
	}

	CVPixelBufferRef pb = NULL;
	CVReturn ret = CVPixelBufferPoolCreatePixelBuffer(NULL, [_assetWriterInputPixelBufferAdaptor pixelBufferPool], &pb);
	if (ret != kCVReturnSuccess)
	{
		VUserLog("Error: Couldn't get buffer from pool: %d", ret);
		return;
	}
	VuoDefer(^{ CVPixelBufferRelease(pb); });

	ret = CVPixelBufferLockBaseAddress(pb, 0);
	if (ret != kCVReturnSuccess)
	{
		VUserLog("Error locking buffer: %d", ret);
		return;
	}

	unsigned char *bytes = (unsigned char *)CVPixelBufferGetBaseAddress(pb);
	if (!bytes)
	{
		VUserLog("Error getting buffer base address.");
		ret = CVPixelBufferUnlockBaseAddress(pb, 0);
		if (ret != kCVReturnSuccess)
			VUserLog("Error unlocking buffer: %d", ret);
		return;
	}

	unsigned int bytesPerRow = CVPixelBufferGetBytesPerRow(pb);

	// Flip the image data (OpenGL returns flipped data, but CVPixelBufferRef assumes it is not flipped),
	// while copying it into `bytes`.
	for (unsigned long y = 0; y < height; ++y)
		memcpy(bytes + bytesPerRow * (height - y - 1), sourceBytes + width * y * 4, width * 4);

	ret = CVPixelBufferUnlockBaseAddress(pb, 0);
	if (ret != kCVReturnSuccess)
		VUserLog("Error unlocking buffer: %d", ret);


	if (CMTimeCompare(time, _priorFrameTime) <= 0)
	{
//		VLog("Warning: Same or earlier time than prior frame; sliding this frame back.");
		time = _priorFrameTime;
		++time.value;
	}


	if (![_assetWriterInputPixelBufferAdaptor appendPixelBuffer:pb withPresentationTime:time])
		VUserLog("Error appending buffer: %s", [[_assetWriter.error description] UTF8String]);

	_priorFrameTime = time;
}

static void VuoWindowRecorder_doNothingCallback(VuoImage imageToFree)
{
}

/**
 * Appends `vis` to the movie file.
 *
 * @threadAny
 */
- (void)saveImage:(VuoIoSurface)vis
{
	if (!vis)
	{
		VUserLog("Error: NULL IOSurface. Skipping frame.");
		return;
	}

	unsigned short width = VuoIoSurfacePool_getWidth(vis);
	unsigned short height = VuoIoSurfacePool_getHeight(vis);

	if (width == 0 || height == 0)
	{
		VUserLog("Error: Invalid viewport size %dx%d. Skipping frame.", width, height);
		return;
	}

	double t0 = VuoLogGetElapsedTime();
	if (!_stopping)
		dispatch_sync(_queue, ^{
			if (!_stopping)
			{
				bool applyWatermark = VuoIsTrial();
				VuoImage image = VuoImage_makeClientOwnedGlTextureRectangle(VuoIoSurfacePool_getTexture(vis), GL_RGBA8, width, height, VuoWindowRecorder_doNothingCallback, NULL);
				VuoRetain(image);

				if (!applyWatermark && width == _originalWidth && height == _originalHeight)
				{
					// rdar://23547737 — glGetTexImage() returns garbage for OpenGL textures backed by IOSurfaces
					VuoImage copiedImage = VuoImage_makeCopy(image, false);
					VuoRetain(copiedImage);
					VuoRelease(image);
					image = copiedImage;
				}
				else
				{
					// Resize.
					if (width != _originalWidth || height != _originalHeight)
					{
						VuoImage resizedImage = VuoImageResize_resize(image, _resize, VuoSizingMode_Fit, _originalWidth, _originalHeight);
						if (!resizedImage)
						{
							VUserLog("Error: Failed to resize image.");
							VuoRelease(image);
							return;
						}

						VuoRetain(resizedImage);
						VuoRelease(image);
						image = resizedImage;
					}

					// Watermark.
					if (applyWatermark)
					{
						VuoImage watermarkedImage = VuoImage_watermark(image);
						VuoRetain(watermarkedImage);
						VuoRelease(image);
						image = watermarkedImage;
					}
				}

				// Download from GPU to CPU, and append to movie.
				dispatch_async(_queue, ^{
					double t0 = VuoLogGetElapsedTime();
					const unsigned char *sourceBytes = VuoImage_getBuffer(image, GL_BGRA);
					if (sourceBytes)
					{
						[self appendBuffer:sourceBytes width:_originalWidth height:_originalHeight];
						VuoRelease(image);
						double t1 = VuoLogGetElapsedTime();
						_totalAsyncTime += t1 - t0;
						++_frameCount;
					}
					else
						VUserLog("Error: Couldn't download image.");
				});

				_priorWidth  = width;
				_priorHeight = height;

				double t1 = VuoLogGetElapsedTime();
				_totalSyncTime += t1 - t0;
			}
		});
}

/**
 * Blocks until the movie file has been finalized.
 *
 * @threadAny
 */
- (void)finish
{
	_stopping = true;

	dispatch_sync(_queue, ^{
					  // Fancy dance to ensure -finishWriting isn't called on the main thread, since that needlessly throws a warning.
					  dispatch_semaphore_t finishedWriting = dispatch_semaphore_create(0);
					  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
						  /// @todo Replace with -finishWritingWithCompletionHandler: when we drop Mac OS 10.8 support.
						  if (![_assetWriter finishWriting])
							  VUserLog("Error: %s", [[_assetWriter.error localizedDescription] UTF8String]);
						  dispatch_semaphore_signal(finishedWriting);
					  });
					  dispatch_semaphore_wait(finishedWriting, DISPATCH_TIME_FOREVER);
					  dispatch_release(finishedWriting);
				  });
	dispatch_release(_queue);

	VuoRelease(_resize);

	if (VuoIsDebugEnabled())
	{
		VUserLog("Average render-blocking record time per frame: %g", _totalSyncTime  / _frameCount);
		VUserLog("Average background      record time per frame: %g", _totalAsyncTime / _frameCount);
	}
}

@end
