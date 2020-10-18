/**
 * @file
 * vuo.image.save node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <FreeImage.h>
#include <json-c/json.h>
#pragma clang diagnostic pop
#include <OpenGL/CGLMacro.h>
#include "VuoImageColorDepth.h"
#include "VuoImageFormat.h"
#include "VuoUrl.h"

VuoModuleMetadata({
					 "title" : "Save Image",
					 "keywords" : [
						 "file", "write", "export",
						 "png", "jpg", "jpeg", "tiff", "bmp", "hdr", "exr", "gif", "tga", "targa", "webp"
					 ],
					 "version" : "1.0.2",
					 "dependencies" : [
						 "freeimage",
						 "VuoInteger",
						 "VuoUrl"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SaveSepiaImage.vuo" ]
					 }
				 });

/**
 * Initializes the FreeImage library.
 */
__attribute__((constructor)) static void vuo_image_save_init(void)
{
	FreeImage_Initialise(true);
}

static void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message)
{
	if(fif != FIF_UNKNOWN)
	{
		VUserLog("Export image failed: %s.  Format: %s", message, FreeImage_GetFormatFromFIF(fif));
	}
	else
	{
		VUserLog("Export image failed: %s", message);
	}
}

void nodeEvent
(
		VuoInputData(VuoText, {"default":"~/Desktop/MyImage", "name":"URL", "isSave":true}) url,
		VuoInputData(VuoImage) saveImage,
		VuoInputEvent({"eventBlocking":"none","data":"saveImage"}) saveImageEvent,
		VuoInputData(VuoBoolean, {"default":true, "name":"Overwrite URL"}) overwriteUrl,
		VuoInputData(VuoImageFormat, {"default":"PNG"}) format,
		VuoOutputEvent() done
)
{
	if (!saveImageEvent || saveImage == NULL || VuoText_isEmpty(url))
		return;

	struct json_object *validExtensions = VuoImageFormat_getValidFileExtensions(format);

	// do the dance of the url format
	VuoUrl extensioned_url = VuoUrl_appendFileExtension(url, validExtensions);
	VuoLocal(extensioned_url);

	json_object_put(validExtensions);

	VuoUrl normalized_url = VuoUrl_normalize(extensioned_url, VuoUrlNormalize_forSaving);
	VuoLocal(normalized_url);

	VuoText absolute_path = VuoUrl_getPosixPath(normalized_url);
	VuoLocal(absolute_path);

	// if overwrite file is false, and the file already exists, return.
	if(!overwriteUrl && access(absolute_path, F_OK) != -1)
	{
		return;
	}

	VuoImageColorDepth cd = VuoImage_getColorDepth(saveImage);
	unsigned int depth = 32;
	if (cd == VuoImageColorDepth_16) depth = 64;
	if (cd == VuoImageColorDepth_32) depth = 128;
	VDebugLog("Input format : %s (depth = %dbpc)",VuoGl_stringForConstant(saveImage->glInternalFormat), depth/4);

	unsigned int bufferFormat = GL_BGRA;
	if (saveImage->glInternalFormat == GL_DEPTH_COMPONENT)
		bufferFormat = GL_DEPTH_COMPONENT16;
	else if (saveImage->glInternalFormat == GL_LUMINANCE
		  || saveImage->glInternalFormat == GL_LUMINANCE16F_ARB
		  || saveImage->glInternalFormat == GL_LUMINANCE32F_ARB)
		bufferFormat = GL_R16;	// 1 channel, 16bit integer (just like depth)
	else if (depth == 64
		  || saveImage->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB)
		bufferFormat = GL_RGBA16I_EXT;
	else if (depth == 128)
	{
		// FreeImage only supports 32bpc EXR, HDR, and TIFF.
		if (format == VuoImageFormat_EXR
		 || format == VuoImageFormat_HDR
		 || format == VuoImageFormat_TIFF)
			bufferFormat = GL_RGBA32F_ARB;
		else
			bufferFormat = GL_RGBA16I_EXT;
	}
	const unsigned char *buffer = VuoImage_getBuffer(saveImage, bufferFormat);
	if (!buffer)
	{
		VUserLog("Error: Couldn't get pixels.");
		return;
	}

	int width = saveImage->pixelsWide;
	int height = saveImage->pixelsHigh;

	// https://b33p.net/kosada/node/12362
	// For image formats that support variable transparency, FreeImage expects unpremultiplied colors.
	// For image formats that don't support variable transparency, we should leave colors premultiplied
	// so the output image isn't abnormally bright.
	bool shouldUnpremultiply = true;
	if (format == VuoImageFormat_EXR
	 || format == VuoImageFormat_GIF
	 || format == VuoImageFormat_HDR
	 || format == VuoImageFormat_JPEG)
		shouldUnpremultiply = false;

	FreeImage_SetOutputMessage(FreeImageErrorHandler);
	FIBITMAP *fibmp;
	if (bufferFormat == GL_BGRA)
	{
		VDebugLog("Output format: RGBA 8bpc integer%s", shouldUnpremultiply?" (unpremultiplied)":"");
		const unsigned char *b = (unsigned char *)buffer;
		fibmp = FreeImage_AllocateT(FIT_BITMAP, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		if (shouldUnpremultiply)
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
		bool mono = (saveImage->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB);
		VDebugLog("Output format: RGBA 16bpc integer%s%s", mono?" (mono)":"", shouldUnpremultiply?" (unpremultiplied)":"");
		const unsigned short *b = (const unsigned short *)buffer;
		fibmp = FreeImage_AllocateT(FIT_RGBA16, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		if (shouldUnpremultiply)
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
		bool mono = (saveImage->glInternalFormat == GL_LUMINANCE_ALPHA32F_ARB);
		VDebugLog("Output format: RGBA 32bpc float%s%s", mono?" (mono)":"", shouldUnpremultiply?" (unpremultiplied)":"");
		const float *b = (const float *)buffer;
		fibmp = FreeImage_AllocateT(FIT_RGBAF, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		if (shouldUnpremultiply)
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
		return;
	}

	switch(format)
	{
		case VuoImageFormat_JPEG:
		{
			FIBITMAP* img = FreeImage_ConvertTo24Bits(fibmp);
			FreeImage_Save(FIF_JPEG, img, absolute_path, 0);
			FreeImage_Unload(img);
		}
		break;

		case VuoImageFormat_TIFF:
		{
			FreeImage_Save(FIF_TIFF, fibmp, absolute_path, 0);
		}
		break;

		case VuoImageFormat_BMP:
		{
			// `FreeImage_Save(FIF_BMP, …)` fails when called with >32bpp images, so reduce it first.
			FIBITMAP *img32 = FreeImage_ConvertTo32Bits(fibmp);
			VuoDefer(^{ FreeImage_Unload(img32); });

			FreeImage_Save(FIF_BMP, img32, absolute_path, 0);
		}
		break;

		case VuoImageFormat_HDR:
		{
			FIBITMAP* img = FreeImage_ConvertToRGBF(fibmp);
			FreeImage_Save(FIF_HDR, img, absolute_path, 0);
			FreeImage_Unload(img);
		}
		break;

		case VuoImageFormat_EXR:
		{
			FIBITMAP* img = FreeImage_ConvertToRGBF(fibmp);
			FreeImage_Save(FIF_EXR, img, absolute_path, 0);
			FreeImage_Unload(img);
		}
		break;

		case VuoImageFormat_GIF:
		{
			// GIFs are <=8bpp.  `FreeImage_ConvertTo8Bits()` converts to greyscale,
			// which is probably not what most people will expect when using this node.
			// Use `FreeImage_ColorQuantize()` instead since it tries to retain some colors.

			// `FreeImage_ColorQuantize()` can only deal with 24bit images.
			FIBITMAP *img24 = FreeImage_ConvertTo24Bits(fibmp);
			VuoDefer(^{ FreeImage_Unload(img24); });

			// Quantize to 254 colors, leaving one for transparency.
			FIBITMAP *imgQuant = FreeImage_ColorQuantizeEx(img24, FIQ_WUQUANT, 254, 0, NULL);
			VuoDefer(^{ FreeImage_Unload(imgQuant); });

			unsigned char transparentIndex = 255;
			FreeImage_SetTransparentIndex(imgQuant, transparentIndex);

			// Find fully-transparent pixels in the 32bpp image,
			// and set the corresponding pixel in the 8bpp image to the transparent index.
			FIBITMAP *img32 = FreeImage_ConvertTo32Bits(fibmp);
			VuoDefer(^{ FreeImage_Unload(img32); });
			for (long int y = 0; y < height; ++y)
			{
				unsigned char *rgba    = (unsigned char *)FreeImage_GetScanLine(img32, y);
				unsigned char *indexed = (unsigned char *)FreeImage_GetScanLine(imgQuant, y);
				for (long int x = 0; x < width; ++x)
					if (rgba[x*4 + 3] == 0)
						indexed[x] = transparentIndex;
			}

			FreeImage_Save(FIF_GIF, imgQuant, absolute_path, 0);
		}
		break;

		case VuoImageFormat_TARGA:
		{
			// `FreeImage_Save(FIF_TARGA, …)` fails when called with >32bpp images, so reduce it first.
			FIBITMAP *img32 = FreeImage_ConvertTo32Bits(fibmp);
			VuoDefer(^{ FreeImage_Unload(img32); });

			FreeImage_Save(FIF_TARGA, img32, absolute_path, 0);
		}
		break;

		case VuoImageFormat_WEBP:
		{
			// `FreeImage_Save(FIF_WEBP, …)` fails when called with >32bpp images, so reduce it first.
			FIBITMAP *img32 = FreeImage_ConvertTo32Bits(fibmp);
			VuoDefer(^{ FreeImage_Unload(img32); });

			FreeImage_Save(FIF_WEBP, img32, absolute_path, 0);
		}
		break;

		default: // VuoImageFormat_PNG
		{
			FreeImage_Save(FIF_PNG, fibmp, absolute_path, 0);
		}
		break;
	}

	FreeImage_Unload(fibmp);
}
