/**
 * @file
 * VuoImageGet implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoFreeImage.h"
#include "VuoImageGet.h"

#include "VuoImageCoreGraphics.h"
#include "VuoImageRotate.h"
#include "VuoGlPool.h"
#include "VuoUrl.h"
#include "VuoUrlFetch.h"

#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
/// Support compiling in Objective-C mode (which defines its own BOOL type having a different underlying type than FreeImage's).
#define BOOL FREEIMAGE_BOOL
#include <FreeImage.h>
#undef BOOL
#pragma clang diagnostic pop

#include <OpenGL/CGLMacro.h>

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoImageGet",
	"dependencies" : [
		"VuoFreeImage",
		"VuoImage",
		"VuoImageCoreGraphics",
		"VuoImageRotate",
		"VuoUrlFetch",
	]
});
#endif
}


/**
 * Returns the VuoImage::scaleFactor for the specified URL.
 *
 * If the image has a filename like `image@2x.png`, this function returns 2.
 *
 * If no scale factor suffix is found, returns 1.
 */
float VuoImage_getScaleFactor(const char *url)
{
	VuoText resolvedUrl = VuoUrl_normalize(url, VuoUrlNormalize_default);
	if (!resolvedUrl)
		return 1;
	VuoLocal(resolvedUrl);

	VuoText path;
	if (!VuoUrl_getParts(resolvedUrl, NULL, NULL, NULL, NULL, &path, NULL, NULL))
		return 1;
	VuoLocal(path);

	size_t lastDot = VuoText_findLastOccurrence(path, ".");
	if (lastDot < 6)
		return 1;

	// `@` is URL-escaped as `%40`.
	if (path[lastDot-6] == '%' && path[lastDot-5] == '4' && path[lastDot-4] == '0' && path[lastDot-2] == 'x')
	{
		char c = path[lastDot-3];
		if (c >= '1' && c <= '9')
			return c - '0';
	}

	return 1;
}

/**
 * @ingroup VuoImage
 * Retrieves the image at the specified @c imageURL, and creates a @c VuoImage from it.
 *
 * @threadAny
 */
VuoImage VuoImage_get(const char *imageURL)
{
	if (!imageURL || !strlen(imageURL))
		return NULL;

	void *data;
	unsigned int dataLength;
	if (!VuoUrl_fetch(imageURL, &data, &dataLength))
		return NULL;

	// Decode the memory buffer into a straightforward array of BGRA pixels
	FIBITMAP *dib;
	{
		FIMEMORY *hmem = FreeImage_OpenMemory((BYTE *)data, dataLength);

		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(hmem, 0);
		if (fif == FIF_UNKNOWN)
		{
			// FreeImage couldn't read it; try CoreGraphics.
			CFDataRef cfd = CFDataCreateWithBytesNoCopy(nullptr, (UInt8 *)data, dataLength, kCFAllocatorNull);
			if (cfd)
			{
				CGImageSourceRef cgis = CGImageSourceCreateWithData(cfd, nullptr);
				CFRelease(cfd);
				if (cgis)
				{
					size_t imageIndex = 0;
					CFDictionaryRef properties = CGImageSourceCopyPropertiesAtIndex(cgis, imageIndex, nullptr);
					int32_t orientation = kCGImagePropertyOrientationUp;
					if (properties)
					{
						CFNumberRef orientationCF = static_cast<CFNumberRef>(CFDictionaryGetValue(properties, kCGImagePropertyOrientation));
						CFNumberGetValue(orientationCF, kCFNumberSInt32Type, &orientation);
						CFRelease(properties);
					}

					CGImageRef cgi = CGImageSourceCreateImageAtIndex(cgis, imageIndex, nullptr);
					CFRelease(cgis);
					if (cgi)
					{
						VuoImage vi = VuoImage_makeFromCGImage(cgi);
						if (vi)
							vi->scaleFactor = VuoImage_getScaleFactor(imageURL);
						CGImageRelease(cgi);
						FreeImage_CloseMemory(hmem);
						free(data);

						if (orientation != kCGImagePropertyOrientationUp)
						{
							VuoImageRotate rotator = VuoImageRotate_make();
							VuoLocal(rotator);

							VuoReal angleInDegrees = 0;
							if (orientation == kCGImagePropertyOrientationDown)
								angleInDegrees = 180;
							else if (orientation == kCGImagePropertyOrientationLeft)
								angleInDegrees = 90;
							else if (orientation == kCGImagePropertyOrientationRight)
								angleInDegrees = -90;

							VuoImage rotatedImage = VuoImageRotate_rotate(vi, rotator, angleInDegrees, true);
							VuoRetain(vi);
							VuoRelease(vi);
							vi = rotatedImage;
						}

						return vi;
					}
				}
			}

			VUserLog("Error: '%s': FreeImage couldn't determine the image type. The file might not be an image, or it might be corrupted.", imageURL);
			FreeImage_CloseMemory(hmem);
			free(data);
			return NULL;
		}
		if (!FreeImage_FIFSupportsReading(fif))
		{
			VUserLog("Error: '%s': FreeImage can't read this type of image.", imageURL);
			FreeImage_CloseMemory(hmem);
			free(data);
			return NULL;
		}

		dib = FreeImage_LoadFromMemory(fif, hmem, JPEG_EXIFROTATE);
		FreeImage_CloseMemory(hmem);

		if (!dib)
		{
			VUserLog("Error: '%s': FreeImage couldn't read this image. The file might be corrupted.", imageURL);
			free(data);
			return NULL;
		}
	}

	VuoImage vuoImage = VuoFreeImage_convertFreeImageToVuoImage(dib, data, imageURL);
	if (!vuoImage)
		return NULL;

	vuoImage->scaleFactor = VuoImage_getScaleFactor(imageURL);

	return vuoImage;
}
