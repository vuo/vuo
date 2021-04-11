/**
 * @file
 * ExampleImageFilterCoreImage implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "node.h"
#import "ExampleImageFilterCoreImage.h"

#import <CoreImage/CoreImage.h>

@interface ExampleImageFilterCoreImage : NSObject
@property CGLContextObj cgl_ctx;
@property GLuint outputFramebuffer;
@property CGColorSpaceRef colorSpace;
@property(retain) CIContext *cic;
@end

@implementation ExampleImageFilterCoreImage
@synthesize cgl_ctx = cgl_ctx;

- (id)initWithCGLContext:(CGLContextObj)cglContext
{
	if (self = [super init])
	{
		self.cgl_ctx = cglContext;

		glGenFramebuffers(1, &_outputFramebuffer);

		_colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		self.cic = [CIContext contextWithCGLContext:cglContext
									pixelFormat:CGLGetPixelFormat(cglContext)
									 colorSpace:_colorSpace
										options:nil];
#pragma clang diagnostic pop
	}
	return self;
}

- (VuoImage)processImage:(VuoImage)inputImage position:(VuoPoint2d)position radius:(float)radius angle:(float)angle
{
	VuoShader_resetContext(cgl_ctx);

	size_t width  = inputImage->pixelsWide;
	size_t height = inputImage->pixelsHigh;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	CIImage *inputCIImage = [CIImage imageWithTexture:inputImage->glTextureName
												 size:CGSizeMake(width, height)
											  flipped:NO
											  options:@{
												  kCIImageTextureFormat: @(inputImage->glInternalFormat),
												  kCIImageTextureTarget: @(inputImage->glTextureTarget),
												  kCIImageColorSpace: (id)_colorSpace,
											  }];
#pragma clang diagnostic pop

	CIFilter *twirlFilter = [CIFilter filterWithName:@"CITwirlDistortion"];
	[twirlFilter setValue:inputCIImage forKey:kCIInputImageKey];
	[twirlFilter setValue:[CIVector vectorWithX:position.x Y:position.y] forKey:kCIInputCenterKey];
	[twirlFilter setValue:@(angle) forKey:kCIInputAngleKey];
	[twirlFilter setValue:@(radius) forKey:kCIInputRadiusKey];
	CIImage *outputCIImage = twirlFilter.outputImage;

	GLuint outputTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_Allocate, GL_TEXTURE_2D, inputImage->glInternalFormat, width, height, GL_RGBA, NULL);
	glBindFramebuffer(GL_FRAMEBUFFER, _outputFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, width, height);
	glDisable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	[_cic drawImage:outputCIImage
			 inRect:CGRectMake(0, 0, width, height)
		   fromRect:CGRectMake(0, 0, width, height)];

	glEnable(GL_CULL_FACE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return VuoImage_make(outputTexture, inputImage->glInternalFormat, width, height);
}

- (void)dealloc
{
	CFRelease(_colorSpace);
	glDeleteFramebuffers(1, &_outputFramebuffer);
	[super dealloc];
}
@end

void *ExampleImageFilterCoreImage_make(CGLContextObj cgl_ctx)
{
	void *t = [[ExampleImageFilterCoreImage alloc] initWithCGLContext:cgl_ctx];
	VuoRegister(t, (DeallocateFunctionType)CFRelease);
	return t;
}

VuoImage ExampleImageFilterCoreImage_processImage(void *t, VuoImage inputImage, VuoPoint2d position, float radius, float angle)
{
	ExampleImageFilterCoreImage *tci = t;
	return [tci processImage:inputImage position:position radius:radius angle:angle];
}
