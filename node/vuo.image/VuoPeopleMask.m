/**
 * @file
 * VuoPeopleMask implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoPeopleMask.h"
#import <Foundation/Foundation.h>
#include <objc/objc-runtime.h>

// Hack: pretend we've already included the SIMD headers that are transitively included by Vision.framework,
// since Vuo's version of Clang has trouble parsing them (`error: call to '__tg_fmin' is ambiguous`).
#define SIMD_COMMON_HEADER
#define SIMD_QUATERNIONS
#define __SIMD_CONVERSION_HEADER__
// End hack.

#import <CoreVideo/CoreVideo.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/CGLMacro.h>
#import <Vision/VNObservation.h>
#import <Vision/VNRequestHandler.h>
#import <Vision/VNStatefulRequest.h>

#import "VuoImageRenderer.h"
#import "VuoOsStatus.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title": "VuoPeopleMask",
	"dependencies": [
		"CoreVideo.framework",
		// "Vision.framework",  // see below
		"VuoImage",
		"VuoImageRenderer",
		"VuoOsStatus",
	]
});
#endif


/**
 * A state object for generating human image masks.
 */
@interface VuoPeopleMask_internal : NSObject
@property(retain) VNSequenceRequestHandler *requestHandler;
@property(retain) id generationRequest;
@end

@implementation VuoPeopleMask_internal
- (id)init
{
	if (self = [super init])
	{
		// Vision.framework is included with macOS 10.13 and later, whereas Vuo supoprts back to 10.12.
		NSBundle *visionFramework = [NSBundle bundleWithPath:@"/System/Library/Frameworks/Vision.framework"];
		NSError *error = nil;
		if (![visionFramework loadAndReturnError:&error])
		{
			VUserLog("Error: Couldn't load macOS Vision.framework: %s", error.localizedDescription.UTF8String);
			return nil;
		}

		Class vnSequenceRequestHandler = NSClassFromString(@"VNSequenceRequestHandler");
		if (!vnSequenceRequestHandler)
		{
			VUserLog("Error: This version of macOS doesn't have an implementation of VNSequenceRequestHandler.");
			return nil;
		}

		_requestHandler = [vnSequenceRequestHandler new];
		if (!_requestHandler)
		{
			VUserLog("Error: [VNSequenceRequestHandler new] failed");
			return nil;
		}

		// Vuo bundles macOS SDK 11, but VNGeneratePersonSegmentationRequest was added in macOS 12, so we need to access it via Objective-C runtime introspection.
		Class vnGeneratePersonSegmentationRequestClass = NSClassFromString(@"VNGeneratePersonSegmentationRequest");
		if (!vnGeneratePersonSegmentationRequestClass)
		{
			VUserLog("Error: This version of macOS doesn't have an implementation of VNGeneratePersonSegmentationRequest.");
			[_requestHandler release];
			return nil;
		}
		_generationRequest = [vnGeneratePersonSegmentationRequestClass new];
		if (!_generationRequest)
		{
			VUserLog("Error: [VNGeneratePersonSegmentationRequest new] failed");
			[_requestHandler release];
			return nil;
		}

		if (![_generationRequest respondsToSelector:@selector(setOutputPixelFormat:)])
		{
			VUserLog("Error: This version of macOS doesn't have an implementation of [VNGeneratePersonSegmentationRequest setOutputPixelFormat:].");
			[_generationRequest release];
			[_requestHandler release];
			return nil;
		}

		((void (*)(id, SEL, OSType))objc_msgSend)(_generationRequest, sel_getUid("setOutputPixelFormat:"), kCVPixelFormatType_OneComponent8);
	}
	return self;
}

static void VuoPeopleMask_IOSurfaceTextureFree(VuoImage i)
{
	VuoGlTexturePool_disuse(VuoGlTexturePool_AllocateIOSurface, i->glTextureTarget, i->glInternalFormat, i->pixelsWide, i->pixelsHigh, i->glTextureName);
}

- (VuoImage)processIOSurface:(IOSurfaceRef)sourceIOSurface image:(VuoImage)sourceImage quality:(VuoReal)quality
{
	int qualityLevel;
	if (quality > 0.66)
		qualityLevel = 0;  // VNGeneratePersonSegmentationRequestQualityLevelAccurate
	else if (quality > 0.33)
		qualityLevel = 1;  // VNGeneratePersonSegmentationRequestQualityLevelBalanced
	else
		qualityLevel = 2;  // VNGeneratePersonSegmentationRequestQualityLevelFast

	if ([_generationRequest respondsToSelector:@selector(setQualityLevel:)])
		((void (*)(id, SEL, int))objc_msgSend)(_generationRequest, sel_getUid("setQualityLevel:"), qualityLevel);


	CVPixelBufferRef pixelBuffer = NULL;
	CVReturn surfRet = CVPixelBufferCreateWithIOSurface(kCFAllocatorDefault, sourceIOSurface, NULL, &pixelBuffer);
	if (surfRet != kCVReturnSuccess)
	{
		char *description = VuoOsStatus_getText(surfRet);
		VUserLog("Error: CVPixelBufferCreateWithIOSurface failed: %s", description);
		free(description);
	}

	NSError *error = nil;
	if (![_requestHandler performRequests:@[_generationRequest] onCVPixelBuffer:pixelBuffer error:&error])
	{
		VUserLog("Error: [VNSequenceRequestHandler performRequests] failed: %s", [error.description cStringUsingEncoding:NSUTF8StringEncoding]);
		CVPixelBufferRelease(pixelBuffer);
		return nil;
	}

	if (![_generationRequest respondsToSelector:@selector(results)])
	{
		VUserLog("Error: This version of macOS doesn't have an implementation of [VNGeneratePersonSegmentationRequest results].");
		CVPixelBufferRelease(pixelBuffer);
		return nil;
	}

	NSArray *results = [_generationRequest performSelector:@selector(results)];
	if (!results)
	{
		VUserLog("Error: [VNGeneratePersonSegmentationRequest results] failed.");
		CVPixelBufferRelease(pixelBuffer);
		return nil;
	}
	if (results.count == 0)
	{
		VUserLog("Error: VNGeneratePersonSegmentationRequest returned an empty result.");
		CVPixelBufferRelease(pixelBuffer);
		return nil;
	}
	if (results.count > 1)
	{
		VUserLog("Error: VNGeneratePersonSegmentationRequest returned multiple results.");
		CVPixelBufferRelease(pixelBuffer);
		return nil;
	}

	VNPixelBufferObservation *pbo = results[0];
	CVPixelBufferRef observationPixelBuffer = pbo.pixelBuffer;
	if (!observationPixelBuffer)
	{
		VUserLog("Error: VNGeneratePersonSegmentationRequest's result was an empty pixelbuffer.");
		CVPixelBufferRelease(pixelBuffer);
		return nil;
	}

	IOSurfaceRef observationIOSurface = CVPixelBufferGetIOSurface(observationPixelBuffer);
	if (!observationIOSurface)
	{
		VUserLog("Error: VNGeneratePersonSegmentationRequest's pixelbuffer doesn't have an IOSurface.");
		CVPixelBufferRelease(pixelBuffer);
		return nil;
	}

	// In a single render pass:
	// 1. convert from GL_TEXTURE_RECTANGLE_ARB to GL_TEXTURE_2D
	// 2. stretch to match the aspect ratio of the source image
	// 3. blend with the source image
	__block VuoImage outputImage = NULL;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		size_t observationWidth  = IOSurfaceGetWidth(observationIOSurface);
		size_t observationHeight = IOSurfaceGetHeight(observationIOSurface);
		GLuint observationTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_AllocateIOSurface, GL_TEXTURE_RECTANGLE_ARB, GL_RED, observationWidth, observationHeight, GL_RED, observationIOSurface);
		if (!observationTexture)
		{
			VUserLog("Error: Couldn't allocate texture.");
			VGL();
			return;
		}

		VuoImage observationImageRect = VuoImage_makeClientOwnedGlTextureRectangle(observationTexture, GL_RED, observationWidth, observationHeight, VuoPeopleMask_IOSurfaceTextureFree, NULL);
		observationImageRect->glTextureTarget = GL_TEXTURE_RECTANGLE_ARB;
		VuoLocal(observationImageRect);

		const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
			\n#include "VuoGlslAlpha.glsl"

			// Inputs
			uniform sampler2D sourceImage;
			uniform sampler2DRect observationImageRect;
			uniform vec2 observationImageRectSize;
			varying vec2 fragmentTextureCoordinate;

			void main()
			{
				vec4 sourceSample = VuoGlsl_sample(sourceImage, fragmentTextureCoordinate);
				vec4 observationSample = VuoGlsl_sampleRect(observationImageRect, fragmentTextureCoordinate * observationImageRectSize);
				gl_FragColor = sourceSample * observationSample.rrrr;
			}
		);

		VuoShader shader = VuoShader_make("VuoPeopleMask");
		VuoLocal(shader);
		VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
		VuoShader_setUniform_VuoImage(shader, "sourceImage", sourceImage);
		VuoShader_setUniform_VuoImage(shader, "observationImageRect", observationImageRect);
		VuoShader_setUniform_VuoPoint2d(shader, "observationImageRectSize", VuoPoint2d_make(observationImageRect->pixelsWide, observationImageRect->pixelsHigh));
		VuoShader_setTransparent(shader, true);

		outputImage = VuoImageRenderer_render(shader, sourceImage->pixelsWide, sourceImage->pixelsHigh, VuoImage_getColorDepth(sourceImage));
	});

	CVPixelBufferRelease(pixelBuffer);
	return outputImage;
}

- (void)dealloc
{
	[_generationRequest release];
	[_requestHandler release];

	[super dealloc];
}
@end

/**
 * Creates a state object for generating human image masks.
 */
VuoPeopleMask VuoPeopleMask_make(void)
{
	void *pi = [VuoPeopleMask_internal new];
	VuoRegister(pi, (DeallocateFunctionType)CFRelease);
	return pi;
}

/**
 * Generates a human image mask.
 */
VuoImage VuoPeopleMask_processImage(VuoPeopleMask p, IOSurfaceRef sourceIOSurface, VuoImage sourceImage, VuoReal quality)
{
	@autoreleasepool {
		VuoPeopleMask_internal *pi = p;
		return [pi processIOSurface:sourceIOSurface image:sourceImage quality:quality];
	}
}
