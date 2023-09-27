/**
 * @file
 * vuo.image.save node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
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
#include <OpenGL/CGLMacro.h>
#include "VuoImageColorDepth.h"
#include "VuoImageFormat.h"
#include "VuoFreeImage.h"
#include "VuoUrl.h"

VuoModuleMetadata({
					 "title" : "Save Image",
					 "keywords" : [
						 "file", "write", "export", "store", "output",
						 "png", "jpg", "jpeg", "tiff", "bmp", "hdr", "exr", "gif", "tga", "targa", "webp"
					 ],
					 "version" : "1.0.3",
					 "dependencies" : [
						 "VuoFreeImage",
						 "VuoInteger",
						 "VuoUrl"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SaveSepiaImage.vuo" ]
					 }
				 });

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


	int width = saveImage->pixelsWide;
	int height = saveImage->pixelsHigh;

	// FreeImage only supports writing float to EXR, HDR, and TIFF formats.
	bool allowFloat = (format == VuoImageFormat_EXR
					|| format == VuoImageFormat_HDR
					|| format == VuoImageFormat_TIFF);

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

	FIBITMAP *fibmp = VuoFreeImage_convertVuoImageToFreeImage(saveImage, allowFloat, shouldUnpremultiply);

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
