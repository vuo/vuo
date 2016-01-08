/**
 * @file
 * vuo.image.save node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
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
 * Given a filename and extension, returns a new string guaranteed to have the extension.
 */
char* appendFileExtensionIfNecessary(const char* filename, const char* extension)
{
	char* curExtension = strrchr(filename, '.');

	if(curExtension != NULL && strcmp(curExtension + 1, extension) == 0)
	{
		return strdup(filename);
	}
	else
	{
		size_t buf_size = strlen(filename) + strlen(extension) + 2;
		char* newfilepath = (char*)malloc(buf_size * sizeof(char));
		snprintf(newfilepath, buf_size, "%s.%s", filename, extension);
		return newfilepath;
	}
}

void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message)
{
	if(fif != FIF_UNKNOWN) 
	{
		VLog("Export image failed: %s.  Format: %s", message, FreeImage_GetFormatFromFIF(fif));
	}
	else
	{
		VLog("Export image failed: %s", message);
	}
}

void nodeEvent
(
		VuoInputData(VuoText, {"default":"MyImage", "name":"URL"}) url,
		VuoInputData(VuoBoolean, {"default":true, "name":"Overwrite URL"}) overwriteUrl,
		VuoInputData(VuoImageFormat, {"default":"png"}) format,
		VuoInputData(VuoImage) saveImage,
		VuoInputEvent({"eventBlocking":"none","data":"saveImage"}) saveImageEvent
)
{
	if(!saveImageEvent || saveImage == NULL)
		return;

	// make sure the file path has the correct extension
	char* extension = VuoImageFormat_getExtension(format);
	char* path = appendFileExtensionIfNecessary(url, extension);
	free(extension);

	// do the dance of the url format
	VuoUrl extensioned_url = VuoText_make(path);
	VuoRetain(extensioned_url);
	free(path);

	VuoUrl normalized_url = VuoUrl_normalize(extensioned_url, false);
	VuoRetain(normalized_url);
	VuoRelease(extensioned_url);
	
	VuoUrl absolute_path = VuoText_replace(normalized_url, "file://", "");
	VuoRetain(absolute_path);
	VuoRelease(normalized_url);

	// if overwrite file is false, and the file already exists, return.
	if(!overwriteUrl && access(absolute_path, F_OK) != -1)
	{
		return;
	}

	FreeImage_Initialise(false);
	FreeImage_SetOutputMessage(FreeImageErrorHandler);

	unsigned char *buffer = VuoImage_copyBuffer(saveImage, GL_BGRA);

	int width = saveImage->pixelsWide;
	int height = saveImage->pixelsHigh;

	unsigned int depth = VuoImage_getColorDepth(saveImage) == VuoImageColorDepth_8 ? 32 : 64;

	FIBITMAP* fibmp = FreeImage_ConvertFromRawBits(buffer, width, height, width * 4, depth, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false);

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
	default:
		{
			FreeImage_Save(FIF_PNG, fibmp, absolute_path, 0);
		}
		break;
	}

	VuoRelease(absolute_path);
	
	FreeImage_Unload(fibmp);

	FreeImage_DeInitialise();
}
