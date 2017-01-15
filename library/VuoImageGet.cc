/**
 * @file
 * VuoImageGet implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoImageGet.h"
#include "VuoUrlFetch.h"

#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <FreeImage.h>
#pragma clang diagnostic pop

#include <OpenGL/CGLMacro.h>


extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageGet",
					 "dependencies" : [
						 "VuoImage",
						 "VuoUrlFetch",
						 "FreeImage"
					 ]
				 });
#endif
}


/**
 * Initializes the FreeImage library.
 */
__attribute__((constructor)) static void VuoImageGet_init(void)
{
	FreeImage_Initialise(true);
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
	GLuint format;
	VuoImageColorDepth colorDepth;
	unsigned char *pixels;
	{
		FIMEMORY *hmem = FreeImage_OpenMemory((BYTE *)data, dataLength);

		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(hmem, 0);
		if (fif == FIF_UNKNOWN)
		{
			VUserLog("Error: '%s': Couldn't determine image type.", imageURL);
			return NULL;
		}
		if (!FreeImage_FIFSupportsReading(fif))
		{
			VUserLog("Error: '%s': This image type doesn't support reading.", imageURL);
			return NULL;
		}

		dib = FreeImage_LoadFromMemory(fif, hmem, 0);
		FreeImage_CloseMemory(hmem);

		if (!dib)
		{
			VUserLog("Error: '%s': Failed to read image.", imageURL);
			return NULL;
		}

		const FREE_IMAGE_TYPE type = FreeImage_GetImageType(dib);
		const unsigned int bpp = FreeImage_GetBPP(dib);
		const FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(dib);

		if (type == FIT_FLOAT
		 || type == FIT_DOUBLE)
		{
			// If it is > 8bpc greyscale, convert to float.
			colorDepth = VuoImageColorDepth_16;
			format = GL_LUMINANCE;
		}
		else if (type == FIT_RGB16
			  || type == FIT_RGBF)
		{
			// If it is > 8bpc WITHOUT an alpha channel, convert to float.
			colorDepth = VuoImageColorDepth_16;
			format = GL_RGB;

			FIBITMAP *dibFloat = FreeImage_ConvertToRGBF(dib);
			FreeImage_Unload(dib);
			dib = dibFloat;
		}
		else if (type == FIT_BITMAP && bpp == 24)
		{
			// Fast path for RGB images — no need to add a fully-opaque alpha channel.
			colorDepth = VuoImageColorDepth_8;
			format = GL_BGR;
		}
		else
		{
			// Upload other images as 8bpc.
			// If it is > 8bpc WITH an alpha channel, convert to 8bpc (since FreeImage doesn't yet support converting to RGBAF).
			colorDepth = VuoImageColorDepth_8;
			format = GL_BGRA;

			if (colorType == FIC_MINISBLACK && bpp == 16)
			{
				// Truncate 16bpc grey images to 8bpc, since `FreeImage_ConvertTo32Bits()` alone doesn't do this.
				FIBITMAP *dibConverted = FreeImage_ConvertTo32Bits(FreeImage_ConvertToStandardType(dib));
				FreeImage_Unload(dib);
				dib = dibConverted;
			}

			if (bpp != 32)
			{
//				VLog("Converting %d bpp to 32 bpp.", bpp);
				FIBITMAP *dibConverted = FreeImage_ConvertTo32Bits(dib);
				FreeImage_Unload(dib);
				dib = dibConverted;
			}

			if (!FreeImage_PreMultiplyWithAlpha(dib))
				VUserLog("Warning: Premultiplication failed.");
		}

		pixels = FreeImage_GetBits(dib);
		if (!pixels)
		{
			VUserLog("Error: '%s': Couldn't get pixels from image.", imageURL);
			FreeImage_Unload(dib);
			return NULL;
		}
	}

	unsigned long pixelsWide = FreeImage_GetWidth(dib);
	unsigned long pixelsHigh = FreeImage_GetHeight(dib);

	VuoImage vuoImage = VuoImage_makeFromBuffer(pixels, format, pixelsWide, pixelsHigh, colorDepth, ^(void *buffer){
		FreeImage_Unload(dib);
		free(data);
	});

	return vuoImage;
}
