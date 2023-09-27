/**
 * @file
 * VuoImageCoreGraphics implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageCoreGraphics.h"

#include "module.h"

#include <OpenGL/CGLMacro.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoImageCoreGraphics",
	"dependencies" : [
		"ApplicationServices.framework",
	]
});
#endif

/**
 * Converts a `CGImageRef` to a VuoImage.
 */
VuoImage VuoImage_makeFromCGImage(CGImageRef cgi)
{
	if (!cgi)
		return NULL;

	size_t width  = CGImageGetWidth(cgi);
	size_t height = CGImageGetHeight(cgi);
	if (width < 1 || height < 1)
		return NULL;

	size_t bitsPerComponent = CGImageGetBitsPerComponent(cgi);
	size_t bitsPerPixel = CGImageGetBitsPerPixel(cgi);
	CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgi);
	CGColorSpaceModel colorModel = CGColorSpaceGetModel(colorSpace);
	CGBitmapInfo bi = CGImageGetBitmapInfo(cgi);
	GLuint format;
	if (bitsPerComponent == 8 && bitsPerPixel == 8 && colorModel == kCGColorSpaceModelMonochrome)
	{
		if (bi != 0)
		{
			VUserLog("Error: The CGImage has an unsupported alpha/byteorder: 0x%x", bi);
			return NULL;
		}

		format = GL_LUMINANCE;
	}
	else if (bitsPerComponent == 8 && bitsPerPixel == 32 && colorModel == kCGColorSpaceModelRGB)
	{
		if (bi != (kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little)
		 && bi != (kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrderDefault)
		 && bi != (kCGImageAlphaNoneSkipLast       | kCGBitmapByteOrderDefault)
		 && bi != (kCGImageAlphaLast               | kCGBitmapByteOrderDefault))
		{
			VUserLog("Error: The CGImage has an unsupported alpha/byteorder: 0x%x", bi);
			return NULL;
		}

		format = GL_BGRA;
		if (bi & kCGImageAlphaNoneSkipLast)
			format = GL_RGBA;
	}
	else
	{
		VUserLog("Error: The CGImage has an unsupported format: bitsPerComponent=%zu bitsPerPixel=%zu colorModel=%d", bitsPerComponent, bitsPerPixel, colorModel);
		return NULL;
	}

	CFDataRef dataFromImageDataProvider = CGDataProviderCopyData(CGImageGetDataProvider(cgi));
	if (!dataFromImageDataProvider)
	{
		VUserLog("Error: Couldn't copy data from the CGImage.");
		return NULL;
	}

	GLubyte *bitmapData = (GLubyte *)CFDataGetBytePtr(dataFromImageDataProvider);
	if (!bitmapData)
	{
		VUserLog("Error: Couldn't get bitmap data from the CGImage.");
		CFRelease(dataFromImageDataProvider);
		return NULL;
	}

	// Flip the image data (CGImage is unflipped, but OpenGL/VuoImage_makeFromBuffer() expect flipped).
	size_t bytesPerPixel = bitsPerPixel / 8;
	size_t bytesPerRow = CGImageGetBytesPerRow(cgi);
	char *flippedBitmapData = (char *)malloc(bytesPerRow * height);
	for (unsigned long y = 0; y < height; ++y)
		memcpy(flippedBitmapData + width * bytesPerPixel * (height - y - 1), bitmapData + bytesPerRow * y, width * bytesPerPixel);

	CFRelease(dataFromImageDataProvider);

	return VuoImage_makeFromBuffer(flippedBitmapData, format, width, height, VuoImageColorDepth_8, ^(void *buffer){ free(flippedBitmapData); });
}
