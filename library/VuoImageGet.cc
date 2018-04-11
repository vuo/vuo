/**
 * @file
 * VuoImageGet implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
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
 * Logs error messages coming from FreeImage.
 */
static void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message)
{
	if (fif != FIF_UNKNOWN)
		VUserLog("Error: %s (%s format)", message, FreeImage_GetFormatFromFIF(fif));
	else
		VUserLog("Error: %s", message);
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
	VuoText resolvedUrl = VuoUrl_normalize(url, false);
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
	FreeImage_SetOutputMessage(FreeImageErrorHandler);
	FIBITMAP *dib;
	GLuint format;
	VuoImageColorDepth colorDepth;
	unsigned char *pixels;
	unsigned long pixelsWide;
	unsigned long pixelsHigh;
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

		dib = FreeImage_LoadFromMemory(fif, hmem, JPEG_EXIFROTATE);
		FreeImage_CloseMemory(hmem);

		if (!dib)
		{
			VUserLog("Error: '%s': Failed to read image.", imageURL);
			return NULL;
		}

		pixelsWide = FreeImage_GetWidth(dib);
		pixelsHigh = FreeImage_GetHeight(dib);

		const FREE_IMAGE_TYPE type = FreeImage_GetImageType(dib);
		const unsigned int bpp = FreeImage_GetBPP(dib);
		const FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(dib);
		VDebugLog("ImageFormat=%d  ImageType=%d  BPP=%d  ImageColorType=%d", fif, type, bpp, colorType);

		if (type == FIT_FLOAT
		 || type == FIT_DOUBLE
		 || type == FIT_UINT16
		 || type == FIT_INT16
		 || type == FIT_UINT32
		 || type == FIT_INT32)
		{
			// If it is > 8bpc greyscale, convert to float.
			colorDepth = VuoImageColorDepth_16;
			format = GL_LUMINANCE;

			FIBITMAP *dibFloat = FreeImage_ConvertToFloat(dib);
			FreeImage_Unload(dib);
			dib = dibFloat;
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
		else if (type == FIT_RGBA16
			  || type == FIT_RGBAF)
		{
			// If it is > 8bpc WITH an alpha channel, convert to float.
			colorDepth = VuoImageColorDepth_16;
			format = GL_RGBA;

			FIBITMAP *dibFloat = FreeImage_ConvertToRGBAF(dib);
			FreeImage_Unload(dib);
			dib = dibFloat;

			// FreeImage_PreMultiplyWithAlpha() only works on 8bpc images, so do it ourself.
			float *pixels = (float *)FreeImage_GetBits(dib);
			if (!pixels)
			{
				VUserLog("Error: '%s': Couldn't get pixels from image.", imageURL);
				FreeImage_Unload(dib);
				return NULL;
			}
			for (int y = 0; y < pixelsHigh; ++y)
				for (int x = 0; x < pixelsWide; ++x)
				{
					float alpha = pixels[(y*pixelsWide + x)*4 + 3];
					pixels[(y*pixelsWide + x)*4 + 0] *= alpha;
					pixels[(y*pixelsWide + x)*4 + 1] *= alpha;
					pixels[(y*pixelsWide + x)*4 + 2] *= alpha;
				}
		}
		else
		{
			// Upload other images as 8bpc.
			colorDepth = VuoImageColorDepth_8;

			if (colorType == FIC_MINISWHITE
			 || colorType == FIC_MINISBLACK)
			{
				format = GL_LUMINANCE;

				FIBITMAP *dibConverted = FreeImage_ConvertTo8Bits(dib);
				FreeImage_Unload(dib);
				dib = dibConverted;
			}
			else if (colorType == FIC_RGB
				 || (colorType == FIC_PALETTE && !FreeImage_IsTransparent(dib)))
			{
				format = GL_BGR;

				FIBITMAP *dibConverted = FreeImage_ConvertTo24Bits(dib);
				FreeImage_Unload(dib);
				dib = dibConverted;
			}
			else if (colorType == FIC_RGBALPHA
				 || (colorType == FIC_PALETTE && FreeImage_IsTransparent(dib)))
			{
				format = GL_BGRA;

				FIBITMAP *dibConverted = FreeImage_ConvertTo32Bits(dib);
				FreeImage_Unload(dib);
				dib = dibConverted;

				if (!FreeImage_PreMultiplyWithAlpha(dib))
					VUserLog("Warning: Premultiplication failed.");
			}
		}

		pixels = FreeImage_GetBits(dib);
		if (!pixels)
		{
			VUserLog("Error: '%s': Couldn't get pixels from image.", imageURL);
			FreeImage_Unload(dib);
			return NULL;
		}
	}

	VuoImage vuoImage = VuoImage_makeFromBuffer(pixels, format, pixelsWide, pixelsHigh, colorDepth, ^(void *buffer){
		FreeImage_Unload(dib);
		free(data);
	});

	vuoImage->scaleFactor = VuoImage_getScaleFactor(imageURL);

	return vuoImage;
}
