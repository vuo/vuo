/**
 * @file
 * VuoWindowRecorder implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoWindowRecorder.h"

#include "VuoWindowOpenGLInternal.h"

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
@property int framesToSkip;			///< Garbage frames to skip capturing.
@property double totalSyncTime;		///< Total time spent blocking rendering.
@property double totalAsyncTime;	///< Total time spent on asyncrhonous tasks (transferring the buffer and submitting to the encoder).
@property (retain) AVAssetWriter *assetWriter;	///< AVAssetWriter
@property (retain) AVAssetWriterInput *assetWriterInput;	///< AVAssetWriterInput
@property (retain) AVAssetWriterInputPixelBufferAdaptor *assetWriterInputPixelBufferAdaptor;	///< AVAssetWriterInputPixelBufferAdaptor
@property (assign) VuoShader resizeShader;	///< Shader for resizing images to fit the output movie size.
@property (assign) VuoGlContext resizeContext;	///< OpenGL context for resizing images to fit the output movie size.
@property (assign) VuoImageRenderer resizeImageRenderer;	///< Image renderer for resizing images to fit the output movie size.
@end


@implementation VuoWindowRecorder

/**
 * Creates a record manager.
 *
 * @threadAny
 */
- (instancetype)initWithWindow:(VuoWindowOpenGLInternal *)window url:(NSURL *)url
{
	if (self = [super init])
	{
		// AVAssetWriter fails if the destination path already exists.
		// Since the user might choose to replace the existing file,
		// delete it if it already exists.
		if ([[NSFileManager defaultManager] fileExistsAtPath:[url path]])
		{
			NSError *error;
			if (![[NSFileManager defaultManager] removeItemAtURL:url error:&error])
			{
				char *errorString = VuoLog_copyCFDescription(error);
				VUserLog("Couldn't replace movie file: %s", errorString);
				return nil;
			}
		}

		self.queue = dispatch_queue_create("org.vuo.VuoWindowRecorder", NULL);

		self.first = true;
		self.priorFrameTime = CMTimeMake(-1, TIMEBASE);

		{
			CGLContextObj cgl_ctx = [[[window glView] openGLContext] CGLContextObj];
			GLint viewport[4];
			glGetIntegerv(GL_VIEWPORT, viewport);
			self.originalWidth  = self.priorWidth  = viewport[2];
			self.originalHeight = self.priorHeight = viewport[3];
			self.framesToSkip   = 0;
		}


		// Set up resize shader.
		{
			self.resizeShader = VuoImageResize_makeShader();
			VuoRetain(self.resizeShader);

			// Needs its own context since VuoImageRenderer changes the glViewport.
			self.resizeContext = VuoGlContext_use();

			self.resizeImageRenderer = VuoImageRenderer_make(self.resizeContext);
			VuoRetain(self.resizeImageRenderer);
		}


		NSError *e = nil;
		self.assetWriter = [AVAssetWriter assetWriterWithURL:url fileType:AVFileTypeQuickTimeMovie error:&e];

		NSDictionary *videoSettings = @{
			AVVideoCodecKey: AVVideoCodecH264,
			AVVideoWidthKey: [NSNumber numberWithInt:self.originalWidth],
			AVVideoHeightKey: [NSNumber numberWithInt:self.originalHeight]
		};


		self.assetWriterInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
		self.assetWriterInput.expectsMediaDataInRealTime = YES;



		NSDictionary *pa = @{
			(NSString *)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithInt:kCVPixelFormatType_32BGRA],
			(NSString *)kCVPixelBufferWidthKey: [NSNumber numberWithInt:self.originalWidth],
			(NSString *)kCVPixelBufferHeightKey: [NSNumber numberWithInt:self.originalHeight],
		};

		self.assetWriterInputPixelBufferAdaptor = [AVAssetWriterInputPixelBufferAdaptor
				assetWriterInputPixelBufferAdaptorWithAssetWriterInput:self.assetWriterInput
				sourcePixelBufferAttributes:pa];

		if (![self.assetWriter canAddInput:self.assetWriterInput])
		{
			VUserLog("Error adding AVAssetWriterInput: %s", [[self.assetWriter.error description] UTF8String]);
			return nil;
		}
		[self.assetWriter addInput:self.assetWriterInput];

		if (![self.assetWriter startWriting])
		{
			VUserLog("Error starting writing: %s", [[self.assetWriter.error description] UTF8String]);
			return nil;
		}
		[self.assetWriter startSessionAtSourceTime:CMTimeMake(0, TIMEBASE)];

		{
			CGLContextObj cgl_ctx = [[[window glView] openGLContext] CGLContextObj];
			CGLLockContext(cgl_ctx);

			[self captureImageOfContext:cgl_ctx];

			CGLUnlockContext(cgl_ctx);
		}
	}

	return self;
}


/**
 * Appends sourceBytes to the movie.
 */
- (void)appendBuffer:(const unsigned char *)sourceBytes width:(unsigned long)width height:(unsigned long)height
{
	if (self.framesToSkip > 0)
	{
//		VLog("Skipping garbage frame after resize.");
		--self.framesToSkip;
		return;
	}


	double captureTime = VuoLogGetTime() - self.startTime;

	CMTime time;
	if (self.first)
	{
		time = CMTimeMake(0, TIMEBASE);
		self.first = false;
		self.startTime = VuoLogGetTime();
	}
	else
		time = CMTimeMake(captureTime * TIMEBASE, TIMEBASE);


	CVPixelBufferRef pb = NULL;

	if (self.stopping)
		goto done;

	if (!self.assetWriterInput.readyForMoreMediaData)
	{
		VUserLog("Warning: AVFoundation video encoder isn't keeping up. Dropping this frame.");
		goto done;
	}

	CVReturn ret = CVPixelBufferPoolCreatePixelBuffer(NULL, [self.assetWriterInputPixelBufferAdaptor pixelBufferPool], &pb);
	if (ret != kCVReturnSuccess)
	{
		VUserLog("Error: Couldn't get buffer from pool: %d", ret);
		goto done;
	}

	ret = CVPixelBufferLockBaseAddress(pb, 0);
	if (ret != kCVReturnSuccess)
	{
		VUserLog("Error locking buffer: %d", ret);
		goto done;
	}

	unsigned char *bytes = (unsigned char *)CVPixelBufferGetBaseAddress(pb);
	if (!bytes)
	{
		VUserLog("Error getting buffer base address.");
		ret = CVPixelBufferUnlockBaseAddress(pb, 0);
		if (ret != kCVReturnSuccess)
			VUserLog("Error unlocking buffer: %d", ret);
		goto done;
	}

	unsigned int bytesPerRow = CVPixelBufferGetBytesPerRow(pb);

	// Flip the image data (OpenGL returns flipped data, but CVPixelBufferRef assumes it is not flipped),
	// while copying it into `bytes`.
	for (unsigned long y = 0; y < height; ++y)
		memcpy(bytes + bytesPerRow * (height - y - 1), sourceBytes + width * y * 4, width * 4);

	ret = CVPixelBufferUnlockBaseAddress(pb, 0);
	if (ret != kCVReturnSuccess)
		VUserLog("Error unlocking buffer: %d", ret);


	if (CMTimeCompare(time, self.priorFrameTime) <= 0)
	{
//			VLog("Warning: Same or earlier time than prior frame; sliding this frame back.");
		time = self.priorFrameTime;
		++time.value;
	}


	if (![self.assetWriterInputPixelBufferAdaptor appendPixelBuffer:pb withPresentationTime:time])
		VUserLog("Error appending buffer: %s", [[self.assetWriter.error description] UTF8String]);

	self.priorFrameTime = time;

done:
	if (pb)
		CVPixelBufferRelease(pb);
}


/**
 * Helper for @ref captureImageOfContext:.
 * Fast path, for when the viewport is the same size as the output movie.
 *
 * Never applies a watermark.
 *
 * @threadQueue{queue}
 */
- (void)captureCorrectlySizedImageOfContext:(CGLContextObj)cgl_ctx
{
	double t0 = VuoLogGetTime();

	GLint width  = self.originalWidth;
	GLint height = self.originalHeight;

	GLuint pbo;
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, 0, GL_DYNAMIC_READ);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
	GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glReadBuffer(GL_BACK);
	glFlush();
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	double t1 = VuoLogGetTime();

	dispatch_async(self.queue, ^{
		double t2 = VuoLogGetTime();
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);

		GLenum syncResult = glClientWaitSync(sync, 0, NSEC_PER_SEC);

		// The glReadPixels() shouldn't sit in the GPU queue for more than 1 second,
		// so we can assume the window's main context has been killed.
		if (syncResult == GL_TIMEOUT_EXPIRED)
		{
//			VLog("Sync timed out");
			goto done;
		}

		// Not sure why this would happen.
		if (syncResult != GL_CONDITION_SATISFIED && syncResult != GL_ALREADY_SIGNALED)
		{
			char *s = VuoGl_stringForConstant(syncResult);
			VUserLog("%s",s);
			free(s);
			VGL();
			goto done;
		}


		unsigned char *sourceBytes = glMapBuffer(GL_PIXEL_PACK_BUFFER,  GL_READ_ONLY);

		[self appendBuffer:sourceBytes width:width height:height];

		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);


		double t4 = VuoLogGetTime();

		++self.frameCount;
		self.totalSyncTime += t1-t0;
		self.totalAsyncTime += t4-t2;

//		VLog("%f", t4-t0);

done:
		glDeleteSync(sync);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glDeleteBuffers(1, &pbo);

		VuoGlContext_disuse(cgl_ctx);
	});
}


/**
 * Helper for @ref captureImageOfContext:.
 * Slow path, for when the viewport is not the same size as the output movie,
 * and/or when a watermark is needed.
 *
 * @threadQueue{queue}
 */
- (void)captureAndResizeImageOfContext:(CGLContextObj)cgl_ctx width:(GLint)width height:(GLint)height withWatermark:(bool)applyWatermark
{
	double t0 = VuoLogGetTime();

	GLint outputWidth  = self.originalWidth;
	GLint outputHeight = self.originalHeight;


	// Capture.
	VuoImage image = VuoImage_makeFromContextFramebuffer(cgl_ctx);
	VuoRetain(image);


	// Resize.
	if (image->pixelsWide != outputWidth || image->pixelsHigh != outputHeight)
	{
		VuoImage resizedImage = VuoImageResize_resize(image, self.resizeShader, self.resizeImageRenderer, VuoSizingMode_Fit, outputWidth, outputHeight);
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


	// Download from GPU to CPU, and append to movie.
	{
		const unsigned char *sourceBytes = VuoImage_getBuffer(image, GL_BGRA);
		[self appendBuffer:sourceBytes width:outputWidth height:outputHeight];
		VuoRelease(image);
	}


	double t4 = VuoLogGetTime();

	++self.frameCount;
	self.totalSyncTime += t4-t0;

//	VLog("%f", t4-t0);
}

/**
 * Captures the current content of `cgl_ctx` and appends it to the movie file.
 *
 * If necessary, you should lock the context sometime before calling this method, and unlock it sometime after.
 *
 * @threadAny
 */
- (void)captureImageOfContext:(CGLContextObj)cgl_ctx
{
	if (!self.stopping)
		dispatch_sync(self.queue, ^{
			if (!self.stopping)
			{
				GLint viewport[4];
				glGetIntegerv(GL_VIEWPORT, viewport);
				GLint width  = viewport[2];
				GLint height = viewport[3];

				if (width == 0 || height == 0)
				{
					VUserLog("Error: Invalid viewport size %dx%d. Skipping frame.", width, height);
					return;
				}

				bool applyWatermark = VuoIsTrial();

//				if (!applyWatermark && width == self.originalWidth && height == self.originalHeight)
//					[self captureCorrectlySizedImageOfContext:cgl_ctx];
//				else
				{
					// glCopyTexImage2D() returns garbage for the first few frames after starting or resizing the window;
					// avoid capturing the garbage.
					if (self.first || width != self.priorWidth || height != self.priorHeight)
						self.framesToSkip = 8;

					// Capture and discard framesToSkip times, then capture a final good frame.
					for (int i = self.framesToSkip; i >= 0; --i)
						[self captureAndResizeImageOfContext:cgl_ctx width:width height:height withWatermark:applyWatermark];
				}

				self.priorWidth  = width;
				self.priorHeight = height;
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
	self.stopping = true;

	dispatch_sync(self.queue, ^{
					  // Fancy dance to ensure -finishWriting isn't called on the main thread, since that needlessly throws a warning.
					  dispatch_semaphore_t finishedWriting = dispatch_semaphore_create(0);
					  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
						  /// @todo Replace with -finishWritingWithCompletionHandler: when we drop Mac OS 10.8 support.
						  if (![self.assetWriter finishWriting])
							  VUserLog("Error: %s", [[self.assetWriter.error localizedDescription] UTF8String]);
						  dispatch_semaphore_signal(finishedWriting);
					  });
					  dispatch_semaphore_wait(finishedWriting, DISPATCH_TIME_FOREVER);
					  dispatch_release(finishedWriting);
				  });
	dispatch_release(self.queue);

	VuoRelease(self.resizeImageRenderer);
	VuoGlContext_disuse(self.resizeContext);
	VuoRelease(self.resizeShader);

	if (VuoIsDebugEnabled())
	{
		VUserLog("Average render-blocking record time per frame: %g", self.totalSyncTime  / self.frameCount);
		VUserLog("Average background      record time per frame: %g", self.totalAsyncTime / self.frameCount);
	}
}

@end
