/**
 * @file
 * VuoImageGet implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoImageGet.h"
#include "VuoUrl.h"

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
						 "c",
						 "json",
						 "VuoUrl",
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
	if (!strlen(imageURL))
		return NULL;

	void *data;
	unsigned int dataLength;
	if (!VuoUrl_get(imageURL, &data, &dataLength))
	{
		fprintf(stderr, "VuoImage_get() Error: Didn't get any image data.\n");
		return NULL;
	}

	// Decode the memory buffer into a straightforward array of BGRA pixels
	FIBITMAP *dib;
	unsigned char *pixels;
	{
		FIMEMORY *hmem = FreeImage_OpenMemory((BYTE *)data, dataLength);

		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(hmem, 0);
		if (fif == FIF_UNKNOWN)
		{
			fprintf(stderr, "VuoImage_get() Error: Couldn't determine image type.\n");
			return NULL;
		}
		if (!FreeImage_FIFSupportsReading(fif))
		{
			fprintf(stderr, "VuoImage_get() Error: This image type doesn't support reading.\n");
			return NULL;
		}

		dib = FreeImage_LoadFromMemory(fif, hmem, 0);
		FreeImage_CloseMemory(hmem);

		if (!dib)
		{
			fprintf(stderr, "VuoImage_get() Error: Failed to read image.\n");
			return NULL;
		}

		if (FreeImage_GetBPP(dib) != 32)
		{
			//fprintf(stderr, "VuoImage_get() Converting image to 32 bits per pixel.\n");
			FIBITMAP *dibConverted = FreeImage_ConvertTo32Bits(dib);
			FreeImage_Unload(dib);
			dib = dibConverted;
		}

		pixels = FreeImage_GetBits(dib);
		if (!pixels)
		{
			fprintf(stderr, "VuoImage_get() Error: Couldn't get pixels from image.\n");
			FreeImage_Unload(dib);
			return NULL;
		}
	}

	unsigned long pixelsWide = FreeImage_GetWidth(dib);
	unsigned long pixelsHigh = FreeImage_GetHeight(dib);

	VuoImage vuoImage = VuoImage_makeFromBuffer(pixels, GL_BGRA, pixelsWide, pixelsHigh);

	FreeImage_Unload(dib);
	free(data);

	return vuoImage;
}
