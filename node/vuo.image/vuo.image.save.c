/**
 * @file
 * vuo.image.save node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <FreeImage.h>
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
					 "version" : "1.0.1",
					 "dependencies" : [
						 "FreeImage",
						 "VuoInteger",
						 "VuoUrl"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ ]
					 }
				 });

/**
 * Initializes the FreeImage library.
 */
__attribute__((constructor)) static void vuo_image_save_init(void)
{
	FreeImage_Initialise(true);
}

/**
 * Given a filename and extension, returns a new string guaranteed to have the extension.
 */
static char *appendFileExtensionIfNecessary(const char *filename, const VuoImageFormat format)
{
	char* fileSuffix = strrchr(filename, '.');
	char* curExtension = fileSuffix != NULL ? strdup(fileSuffix+1) : NULL;

	if(curExtension != NULL)
		for(char *p = &curExtension[0]; *p; p++) *p = tolower(*p);

	int length;
	char** validExtensions = VuoImageFormat_getValidFileExtensions(format, &length);

	// if the string already has one of the valid file extension suffixes, return.
	for(int i = 0; i < length; i++)
	{
		if(curExtension != NULL && strcmp(curExtension, validExtensions[i]) == 0)
		{
			for(int n = 0; n < length; n++)
				free(validExtensions[n]);
			free(validExtensions);

			free(curExtension);

			return strdup(filename);
		}
	}

	free(curExtension);

	size_t buf_size = strlen(filename) + strlen(validExtensions[0]) + 2;
	char* newfilepath = (char*)malloc(buf_size * sizeof(char));
	snprintf(newfilepath, buf_size, "%s.%s", filename, validExtensions[0]);

	for(int n = 0; n < length; n++)
		free(validExtensions[n]);
	free(validExtensions);

	return newfilepath;
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
		VuoInputData(VuoImageFormat, {"default":"png"}) format,
		VuoOutputEvent() done
)
{
	if(!saveImageEvent || saveImage == NULL)
		return;

	// make sure the file path has the correct extension
	char* path = appendFileExtensionIfNecessary(url, format);

	// do the dance of the url format
	VuoUrl extensioned_url = VuoText_make(path);
	VuoLocal(extensioned_url);
	free(path);

	VuoUrl normalized_url = VuoUrl_normalize(extensioned_url, true);
	VuoLocal(normalized_url);

	VuoText absolute_path = VuoUrl_getPosixPath(normalized_url);
	VuoLocal(absolute_path);

	// if overwrite file is false, and the file already exists, return.
	if(!overwriteUrl && access(absolute_path, F_OK) != -1)
	{
		return;
	}

	unsigned int depth = VuoImage_getColorDepth(saveImage) == VuoImageColorDepth_8 ? 32 : 64;
	VDebugLog("Input format : %s (depth = %dbpc)",VuoGl_stringForConstant(saveImage->glInternalFormat), depth/4);

	unsigned int bufferFormat = GL_BGRA;
	if (saveImage->glInternalFormat == GL_DEPTH_COMPONENT)
		bufferFormat = GL_DEPTH_COMPONENT16;
	else if (saveImage->glInternalFormat == GL_LUMINANCE
		  || saveImage->glInternalFormat == GL_LUMINANCE16F_ARB)
		bufferFormat = GL_R16;	// 1 channel, 16bit integer (just like depth)
	else if (depth == 64
		  || saveImage->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB)
		bufferFormat = GL_RGBA16I_EXT;
	const unsigned char *buffer = VuoImage_getBuffer(saveImage, bufferFormat);
	if (!buffer)
	{
		VUserLog("Error: Couldn't get pixels.");
		return;
	}

	int width = saveImage->pixelsWide;
	int height = saveImage->pixelsHigh;

	FreeImage_SetOutputMessage(FreeImageErrorHandler);
	FIBITMAP *fibmp;
	if (bufferFormat == GL_BGRA)
	{
		VDebugLog("Output format: RGBA 8bpc integer");
		// VuoImage_getBuffer() provides premultiplied colors, but FreeImage expects un-premultiplied.
		const unsigned char *b = (unsigned char *)buffer;
		fibmp = FreeImage_AllocateT(FIT_BITMAP, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
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
	}
	else if (bufferFormat == GL_RGBA16I_EXT)
	{
		bool mono = (saveImage->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB);
		VDebugLog("Output format: RGBA 16bpc integer%s", mono?" (mono)":"");
		// VuoImage_getBuffer() provides premultiplied colors, but FreeImage expects un-premultiplied.
		const unsigned short *b = (const unsigned short *)buffer;
		fibmp = FreeImage_AllocateT(FIT_RGBA16, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
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
		case VuoImageFormat_PNG:
		{
			FreeImage_Save(FIF_PNG, fibmp, absolute_path, 0);
		}
		break;

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

		default:
		{
			FreeImage_Save(FIF_PNG, fibmp, absolute_path, 0);
		}
		break;
	}

	FreeImage_Unload(fibmp);
}
