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
					  "keywords" : [ "file", "write", "png", "jpeg", "tiff", "gif" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "FreeImage",
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
		VuoInputData(VuoImageFormat, {"default":"png"}) format
)
{
	if(!saveImageEvent || saveImage == NULL)
		return;

	// make sure the file path has the correct extension
	char* path = appendFileExtensionIfNecessary(url, format);

	// do the dance of the url format
	VuoUrl extensioned_url = VuoText_make(path);
	VuoRetain(extensioned_url);
	free(path);

	VuoUrl normalized_url = VuoUrl_normalize(extensioned_url, true);
	VuoRetain(normalized_url);
	VuoRelease(extensioned_url);

	VuoText absolute_path = VuoUrl_getPosixPath(normalized_url);
	VuoRetain(absolute_path);
	VuoRelease(normalized_url);

	// if overwrite file is false, and the file already exists, return.
	if(!overwriteUrl && access(absolute_path, F_OK) != -1)
	{
		return;
	}

	unsigned int depth = VuoImage_getColorDepth(saveImage) == VuoImageColorDepth_8 ? 32 : 64;

	unsigned int bufferFormat = GL_BGRA;
	if (saveImage->glInternalFormat == GL_DEPTH_COMPONENT)
		bufferFormat = GL_DEPTH_COMPONENT16;
	else if (depth == 64)
		bufferFormat = GL_RGBA16I_EXT;
	const unsigned char *buffer = VuoImage_getBuffer(saveImage, bufferFormat);
	if (!buffer)
	{
		VUserLog("Error: Couldn't get pixels.");
		return;
	}

	int width = saveImage->pixelsWide;
	int height = saveImage->pixelsHigh;

	FIBITMAP *fibmp;
	if (bufferFormat == GL_BGRA)
	{
		// VuoImage_getBuffer() provides premultiplied colors, but FreeImage expects un-premultiplied.
		const unsigned char *b = (unsigned char *)buffer;
		fibmp = FreeImage_AllocateT(FIT_BITMAP, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		for (long int y = 0; y < height; ++y)
		{
			unsigned char *d = (unsigned char *)FreeImage_GetScanLine(fibmp, y);
			for (long int x = 0; x < width; ++x)
			{
				float alpha = (float)b[3] / 255.;
				d[0] = (float)b[0] / alpha;
				d[1] = (float)b[1] / alpha;
				d[2] = (float)b[2] / alpha;
				d[3] = b[3];
				b += 4;
				d += 4;
			}
		}
	}
	else if (bufferFormat == GL_RGBA16I_EXT)
	{
		// VuoImage_getBuffer() provides premultiplied colors, but FreeImage expects un-premultiplied.
		const unsigned short *b = (const unsigned short *)buffer;
		fibmp = FreeImage_AllocateT(FIT_RGBA16, width, height, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK);
		for (long int y = 0; y < height; ++y)
		{
			unsigned short *d = (unsigned short *)FreeImage_GetScanLine(fibmp, y);
			for (long int x = 0; x < width; ++x)
			{
				float alpha = (float)b[3] / 65535.;
				d[0] = (float)b[2] / alpha;
				d[1] = (float)b[1] / alpha;
				d[2] = (float)b[0] / alpha;
				d[3] = b[3];
				b += 4;
				d += 4;
			}
		}
	}
	else if (bufferFormat == GL_DEPTH_COMPONENT16)
	{
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
			FIBITMAP* img = FreeImage_ConvertTo24Bits(fibmp);
			FreeImage_Save(FIF_TIFF, img, absolute_path, 0);
			FreeImage_Unload(img);
		}
		break;

		case VuoImageFormat_BMP:
		{
			FreeImage_Save(FIF_BMP, fibmp, absolute_path, 0);
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
			FIBITMAP* img = FreeImage_ConvertTo8Bits(fibmp);
			FreeImage_Save(FIF_GIF, img, absolute_path, 0);
			FreeImage_Unload(img);
		}
		break;

		case VuoImageFormat_TARGA:
		{
			FreeImage_Save(FIF_TARGA, fibmp, absolute_path, 0);
		}
		break;

		// @todo https://b33p.net/kosada/node/10022
		// case VuoImageFormat_WEBP:
		// {
		// 	FreeImage_Save(FIF_WEBP, fibmp, absolute_path, 0);
		// }
		// break;

		default:
		{
			FreeImage_Save(FIF_PNG, fibmp, absolute_path, 0);
		}
		break;
	}

	VuoRelease(absolute_path);

	FreeImage_Unload(fibmp);
}
