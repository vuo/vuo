/**
 * @file
 * VuoScreenCapture implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "module.h"
#include "VuoScreenCapture.h"
#include"VuoGlPool.h"

#define NS_RETURNS_INNER_POINTER
#import <AVFoundation/AVFoundation.h>
#import <OpenGL/gl.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoScreenCapture",
					  "dependencies" : [
						  "AVFoundation.framework",
						  "CoreMedia.framework",
						  "CoreVideo.framework"
					  ]
				 });
#endif

@class VuoScreenCaptureDelegate;

/**
 * Data for a screen capture instance.
 */
typedef struct
{
	CVOpenGLTextureCacheRef textureCache;			///< A quick way to convert a CGPixelBuffer to an OpenGL texture.
	CGLContextObj glContext;						///< textureCache's OpenGL context.
	dispatch_queue_t queue;							///< Serializes access to glContext.
	AVCaptureSession *session;						///< Manages capturing the screen.
	VuoScreenCaptureDelegate *delegate;				///< Internal class invoked when a frame is captured.
	void (*capturedImageTrigger)(VuoImage image);	///< The node callback to invoke when a frame is captured.
} VuoScreenCaptureInternal;

/**
 * Does nothing.
 */
static void VuoScreenCapture_freeCallback(VuoImage imageToFree)
{
}

/**
 * Callback to process captured frames.
 */
@interface VuoScreenCaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property VuoScreenCaptureInternal *sci;	///< Internal data.
@end

@implementation VuoScreenCaptureDelegate
/**
 * Processes a frame captured by the AVCaptureSession.
 *
 * @threadQueue{VuoScreenCaptureInternal::queue}
 */
- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

	CVOpenGLTextureRef texture;
	CVOpenGLTextureCacheCreateTextureFromImage(NULL, _sci->textureCache, pixelBuffer, NULL, &texture);

	VuoImage rectImage = VuoImage_makeClientOwnedGlTextureRectangle(
				CVOpenGLTextureGetName(texture),
				GL_RGB,
				CVPixelBufferGetWidth(pixelBuffer),
				CVPixelBufferGetHeight(pixelBuffer),
				VuoScreenCapture_freeCallback, NULL);
	VuoRetain(rectImage);
	VuoImage image = VuoImage_makeCopy(rectImage, CVOpenGLTextureIsFlipped(texture));
	_sci->capturedImageTrigger(image);
	CVOpenGLTextureRelease(texture);
	VuoRelease(rectImage);

	CVOpenGLTextureCacheFlush(_sci->textureCache, 0);
}
@end

/**
 * Releases screen capture objects.  Blocks until all captured frames have been fired.
 */
void VuoScreenCapture_free(void *p)
{
	VuoScreenCaptureInternal *sci = (VuoScreenCaptureInternal *)p;

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [sci->session stopRunning];
				  });
	dispatch_sync(sci->queue, ^{});

	[sci->session release];
	[sci->delegate release];

	if (sci->textureCache)
		CVOpenGLTextureCacheRelease(sci->textureCache);

	dispatch_release(sci->queue);

	VuoGlContext_disuse(sci->glContext);
}

/**
 * Creates a screen capture manager, and starts firing the `capturedImage` callback.
 */
VuoScreenCapture VuoScreenCapture_make(VuoScreen screen, VuoRectangle rectangle, void (*capturedImage)(VuoImage image))
{
	if ( (rectangle.center.x-rectangle.size.x/2.) >= screen.width
	  || (rectangle.center.y-rectangle.size.y/2.) >= screen.height)
		return NULL;

	VuoScreenCaptureInternal *sci = (VuoScreenCaptureInternal *)calloc(1, sizeof(VuoScreenCaptureInternal));
	VuoRegister(sci, VuoScreenCapture_free);

	sci->capturedImageTrigger = capturedImage;

	sci->queue = dispatch_queue_create("org.vuo.VuoScreenCapture", NULL);

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  sci->delegate = [VuoScreenCaptureDelegate new];
					  sci->delegate.sci = sci;

					  sci->session = [AVCaptureSession new];
					  sci->session.sessionPreset = AVCaptureSessionPresetPhoto;

					  {
						  AVCaptureScreenInput *input = [[AVCaptureScreenInput alloc] initWithDisplayID:screen.id];
						  input.minFrameDuration = CMTimeMake(1, 120);

						  double width = rectangle.size.x;
						  if (width < 1)
						  width = screen.width;

						  double height = rectangle.size.y;
						  if (height < 1)
						  height = screen.height;

						  input.cropRect = CGRectMake(
						  (int)(rectangle.center.x - rectangle.size.x/2.),
						  (int)(screen.height - (rectangle.center.y - rectangle.size.y/2.) - height),
						  (int)width,
						  (int)height);
						  //		input.capturesMouseClicks = YES;
						  [sci->session addInput:input];
						  [input release];
					  }

					  {
						  AVCaptureVideoDataOutput *output = [AVCaptureVideoDataOutput new];
						  [output setAlwaysDiscardsLateVideoFrames:YES];
						  [output setSampleBufferDelegate:sci->delegate queue:sci->queue];
						  [sci->session addOutput:output];
						  [output release];
					  }
				  });

	{
		sci->glContext = (CGLContextObj)VuoGlContext_use();
		CGLPixelFormatObj pf = VuoGlContext_makePlatformPixelFormat(false);
		CVReturn ret = CVOpenGLTextureCacheCreate(NULL, NULL, sci->glContext, pf, NULL, &sci->textureCache);
		CGLReleasePixelFormat(pf);

		if (ret != kCVReturnSuccess)
		{
			VUserLog("Error: Couldn't create texture cache: %d", ret);
			VuoScreenCapture_free(sci);
			return NULL;
		}
	}

	{
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [sci->session startRunning];
				  });
	}

	return (VuoScreenCapture)sci;
}
