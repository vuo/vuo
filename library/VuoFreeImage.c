/**
 * @file
 * VuoFreeImage implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
/// Support compiling in Objective-C mode (which defines its own BOOL type having a different underlying type than FreeImage's).
#define BOOL FREEIMAGE_BOOL
#include <FreeImage.h>
#undef BOOL
#pragma clang diagnostic pop

#include "VuoFreeImage.h"
#include "VuoGlPool.h"

#include <OpenGL/CGLMacro.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoFreeImage",
	"dependencies" : [
		"freeimage"
	]
});
#endif

/**
 * Logs error messages coming from FreeImage.
 */
static void VuoFreeImage_error(FREE_IMAGE_FORMAT fif, const char *message)
{
	if (fif != FIF_UNKNOWN)
		VUserLog("Error: %s (%s format)", message, FreeImage_GetFormatFromFIF(fif));
	else
		VUserLog("Error: %s", message);
}

/**
 * Initializes the FreeImage library.
 */
__attribute__((constructor)) static void VuoFreeImage_init(void)
{
	FreeImage_Initialise(true);
	FreeImage_SetOutputMessage(VuoFreeImage_error);
}

/**
 * Returns a `FIBITMAP` containing the pixel data from `image`,
 * or NULL if the image couldn't be converted.
 *
 * If `allowFloat` is true and the input image is `VuoImageColorDepth_32`, this function will return `FIT_RGBAF`.
 * Otherwise it will return an integer format (`FIT_BITMAP`, `FIT_RGBA16`, `FIT_UINT16`).
 *
 * If `unpremultiply` is true, the RGB components are divided by the alpha component
 * (required for `FreeImage_Save` with some formats).
 */
FIBITMAP *VuoFreeImage_convertVuoImageToFreeImage(VuoImage image, bool allowFloat, bool unpremultiply)
{
	if (!image)
		return NULL;

	VuoImageColorDepth cd = VuoImage_getColorDepth(image);
	unsigned int depth = 32;
	if (cd == VuoImageColorDepth_16) depth = 64;
	if (cd == VuoImageColorDepth_32) depth = 128;
	VDebugLog("Input format : %s (depth = %dbpc)", VuoGl_stringForConstant(image->glInternalFormat), depth/4);

	unsigned int bufferFormat = GL_BGRA;
	if (image->glInternalFormat == GL_DEPTH_COMPONENT)
		bufferFormat = GL_DEPTH_COMPONENT16;
	else if (image->glInternalFormat == GL_LUMINANCE
		  || image->glInternalFormat == GL_LUMINANCE16F_ARB
		  || image->glInternalFormat == GL_LUMINANCE32F_ARB)
		bufferFormat = GL_R16;  // 1 channel, 16bit integer (just like depth)
	else if (depth == 64
		  || image->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB)
		bufferFormat = GL_RGBA16I_EXT;
	else if (depth == 128)
	{
		if (allowFloat)
			bufferFormat = GL_RGBA32F_ARB;
		else
			bufferFormat = GL_RGBA16I_EXT;
	}
	const unsigned char *buffer = VuoImage_getBuffer(image, bufferFormat);
	if (!buffer)
	{
		VUserLog("Error: Couldn't get pixels.");
		return NULL;
	}

	int width = image->pixelsWide;
	int height = image->pixelsHigh;

	FIBITMAP *fibmp;
	if (bufferFormat == GL_BGRA)
	{
		VDebugLog("Output format: RGBA 8bpc integer%s", unpremultiply ? " (unpremultiplied)" : "");
		const unsigned char *b = (unsigned char *)buffer;
		fibmp = FreeImage_AllocateT(FIT_BITMAP, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		if (unpremultiply)
			for (long int y = 0; y < height; ++y)
			{
				unsigned char *d = (unsigned char *)FreeImage_GetScanLine(fibmp, y);
				for (long int x = 0; x < width; ++x)
				{
					float alpha = (float)b[3] / 255.;
					// Clamp after un-premultiplying, to ensure the `unsigned char` values don't wrap
					// https://b33p.net/kosada/node/11821
					d[0] = VuoInteger_clamp((float)b[0] / alpha, 0, 255);
					d[1] = VuoInteger_clamp((float)b[1] / alpha, 0, 255);
					d[2] = VuoInteger_clamp((float)b[2] / alpha, 0, 255);
					d[3] = b[3];
					b += 4;
					d += 4;
				}
			}
		else
			memcpy(FreeImage_GetBits(fibmp), buffer, width*height*4);
	}
	else if (bufferFormat == GL_RGBA16I_EXT)
	{
		bool mono = (image->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB);
		VDebugLog("Output format: RGBA 16bpc integer%s%s", mono?" (mono)":"", unpremultiply ? " (unpremultiplied)" : "");
		const unsigned short *b = (const unsigned short *)buffer;
		fibmp = FreeImage_AllocateT(FIT_RGBA16, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		if (unpremultiply)
			for (long int y = 0; y < height; ++y)
			{
				unsigned short *d = (unsigned short *)FreeImage_GetScanLine(fibmp, y);
				for (long int x = 0; x < width; ++x)
				{
					float alpha = (float)b[3] / 65535.;
					// Clamp after un-premultiplying, to ensure the `unsigned short` values don't wrap
					// https://b33p.net/kosada/node/11821
					d[0] = VuoInteger_clamp((float)b[       2] / alpha, 0, 65535);
					d[1] = VuoInteger_clamp((float)b[mono?2:1] / alpha, 0, 65535);
					d[2] = VuoInteger_clamp((float)b[mono?2:0] / alpha, 0, 65535);
					d[3] = b[3];
					b += 4;
					d += 4;
				}
			}
		else
			for (long int y = 0; y < height; ++y)
			{
				unsigned short *d = (unsigned short *)FreeImage_GetScanLine(fibmp, y);
				for (long int x = 0; x < width; ++x)
				{
					d[0] = b[2];
					d[1] = b[mono?2:1];
					d[2] = b[mono?2:0];
					d[3] = b[3];
					b += 4;
					d += 4;
				}
			}
	}
	else if (bufferFormat == GL_RGBA32F_ARB)
	{
		bool mono = (image->glInternalFormat == GL_LUMINANCE_ALPHA32F_ARB);
		VDebugLog("Output format: RGBA 32bpc float%s%s", mono?" (mono)":"", unpremultiply ? " (unpremultiplied)" : "");
		const float *b = (const float *)buffer;
		fibmp = FreeImage_AllocateT(FIT_RGBAF, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		if (unpremultiply)
			for (long int y = 0; y < height; ++y)
			{
				float *d = (float *)FreeImage_GetScanLine(fibmp, y);
				for (long int x = 0; x < width; ++x)
				{
					float alpha = b[3];
					d[0] = b[       2] / alpha;
					d[1] = b[mono?2:1] / alpha;
					d[2] = b[mono?2:0] / alpha;
					d[3] = b[3];
					b += 4;
					d += 4;
				}
			}
		else
			for (long int y = 0; y < height; ++y)
			{
				float *d = (float *)FreeImage_GetScanLine(fibmp, y);
				for (long int x = 0; x < width; ++x)
				{
					d[0] = b[2];
					d[1] = b[mono?2:1];
					d[2] = b[mono?2:0];
					d[3] = b[3];
					b += 4;
					d += 4;
				}
			}
	}
	else if (bufferFormat == GL_DEPTH_COMPONENT16
		  || bufferFormat == GL_R16)
	{
		VDebugLog("Output format: mono 16bpc integer");
		const unsigned short *b = (const unsigned short *)buffer;
		fibmp = FreeImage_AllocateT(FIT_UINT16, width, height, 16, 0, 0, 0);
		for (long int y = 0; y < height; ++y)
		{
			unsigned short *d = (unsigned short *)FreeImage_GetScanLine(fibmp, y);
			memcpy(d, b + y*width, width*2);
		}
	}
	else
	{
		VUserLog("Error: Unknown bufferFormat.");
		return NULL;
	}

	return fibmp;
}

/**
 * Returns a `VuoImage` containing the pixel data from `fi`,
 * or NULL if the image couldn't be converted.
 *
 * This function takes ownership of `fi`;
 * the caller should not modify it or call `FreeImage_Unload` on it.
 * This function will release `fi` even if conversion fails.
 *
 * `dataToFree` (optional) is `free()`d as part of the `VuoImage`'s destructor.
 */
VuoImage VuoFreeImage_convertFreeImageToVuoImage(FIBITMAP *fi, void *dataToFree, const char *imageURL)
{
	if (!fi)
		return NULL;

	// Decode the memory buffer into a straightforward array of BGRA pixels
	GLuint format;
	VuoImageColorDepth colorDepth;
	unsigned char *pixels;
	unsigned long pixelsWide;
	unsigned long pixelsHigh;
	{
		pixelsWide = FreeImage_GetWidth(fi);
		pixelsHigh = FreeImage_GetHeight(fi);

		const FREE_IMAGE_TYPE type            = FreeImage_GetImageType(fi);
		const unsigned int bpp                = FreeImage_GetBPP(fi);
		const FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(fi);
		const FIICCPROFILE *colorProfile      = FreeImage_GetICCProfile(fi);
		FITAG *exifColorSpace                 = NULL;
		FreeImage_GetMetadata(FIMD_EXIF_EXIF, fi, "ColorSpace", &exifColorSpace);
		VDebugLog("ImageType=%d  BPP=%d  colorType=%s  Profile=%s(%d)  EXIF_ColorSpace=%s",
				  type,
				  bpp,
				  colorType == FIC_MINISWHITE ? "minIsWhite" :
					  (colorType == FIC_MINISBLACK ? "minIsBlack" :
					  (colorType == FIC_RGB ? "RGB" :
					  (colorType == FIC_PALETTE ? "indexed" :
					  (colorType == FIC_RGBALPHA ? "RGBA" :
					  (colorType == FIC_CMYK ? "CMYK" : "unknown"))))),
				  colorProfile->flags & FIICC_COLOR_IS_CMYK ? "CMYK" : "RGB",
				  colorProfile->size,
				  FreeImage_TagToString(FIMD_EXIF_EXIF, exifColorSpace, NULL));

		if (type == FIT_FLOAT
		 || type == FIT_DOUBLE
		 || type == FIT_UINT16
		 || type == FIT_INT16
		 || type == FIT_UINT32
		 || type == FIT_INT32)
		{
			// If it is > 8bpc greyscale, convert to float.
			colorDepth = VuoImageColorDepth_32;
			format     = GL_LUMINANCE;

			FIBITMAP *fiFloat = FreeImage_ConvertToFloat(fi);
			FreeImage_Unload(fi);
			fi = fiFloat;
		}
		else if (type == FIT_RGB16
			  || type == FIT_RGBF)
		{
			// If it is > 8bpc WITHOUT an alpha channel, convert to float.
			colorDepth = VuoImageColorDepth_32;
			format     = GL_RGB;

			FIBITMAP *fiFloat = FreeImage_ConvertToRGBF(fi);
			FreeImage_Unload(fi);
			fi = fiFloat;
		}
		else if (type == FIT_RGBA16
			  || type == FIT_RGBAF)
		{
			// If it is > 8bpc WITH an alpha channel, convert to float.
			colorDepth = VuoImageColorDepth_32;
			format     = GL_RGBA;

			FIBITMAP *fiFloat = FreeImage_ConvertToRGBAF(fi);
			FreeImage_Unload(fi);
			fi = fiFloat;

			// FreeImage_PreMultiplyWithAlpha() only works on 8bpc images, so do it ourself.
			float *pixels = (float *)FreeImage_GetBits(fi);
			if (!pixels)
			{
				VUserLog("Error: '%s': FreeImage couldn't get this image's pixel data. The file might be corrupted.", imageURL);
				FreeImage_Unload(fi);
				free(dataToFree);
				return NULL;
			}
			for (int y = 0; y < pixelsHigh; ++y)
				for (int x = 0; x < pixelsWide; ++x)
				{
					float alpha = pixels[(y * pixelsWide + x) * 4 + 3];
					pixels[(y * pixelsWide + x) * 4 + 0] *= alpha;
					pixels[(y * pixelsWide + x) * 4 + 1] *= alpha;
					pixels[(y * pixelsWide + x) * 4 + 2] *= alpha;
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

				FIBITMAP *fiConverted = FreeImage_ConvertTo8Bits(fi);
				FreeImage_Unload(fi);
				fi = fiConverted;
			}
			else if (colorType == FIC_RGB
				  || (colorType == FIC_PALETTE && !FreeImage_IsTransparent(fi)))
			{
				format = GL_BGR;

				FIBITMAP *fiConverted = FreeImage_ConvertTo24Bits(fi);
				FreeImage_Unload(fi);
				fi = fiConverted;
			}
			else if (colorType == FIC_RGBALPHA
				  || (colorType == FIC_PALETTE && FreeImage_IsTransparent(fi)))
			{
				format = GL_BGRA;

				FIBITMAP *fiConverted = FreeImage_ConvertTo32Bits(fi);
				FreeImage_Unload(fi);
				fi = fiConverted;

				if (!FreeImage_PreMultiplyWithAlpha(fi))
					VUserLog("Warning: Premultiplication failed.");
			}
			else
			{
				VUserLog("Error: '%s': Unknown colorType %d.", imageURL, colorType);
				FreeImage_Unload(fi);
				free(dataToFree);
				return NULL;
			}
		}

		pixels = FreeImage_GetBits(fi);
		if (!pixels)
		{
			VUserLog("Error: '%s': FreeImage couldn't get this image's pixel data. The file might be corrupted.", imageURL);
			FreeImage_Unload(fi);
			free(dataToFree);
			return NULL;
		}
	}

	// FreeImage's documentation says "Every scanline is DWORD-aligned."
	// …so round the row stride up to the nearest 4 bytes.
	// See @ref TestVuoImage::testFetchOddStride.
	int bytesPerRow = pixelsWide * VuoGlTexture_getBytesPerPixelForInternalFormat(VuoImageColorDepth_getGlInternalFormat(format, colorDepth));
	bytesPerRow     = (bytesPerRow + 3) & ~0x3;


	void (^freeCallback)(void *buffer) = ^(void *buffer) {
		FreeImage_Unload(fi);
		free(dataToFree);
	};

	VuoImage vuoImage = VuoImage_makeFromBufferWithStride(pixels, format, pixelsWide, pixelsHigh, bytesPerRow, colorDepth, freeCallback);
	if (!vuoImage)
		freeCallback(NULL);

	return vuoImage;
}
