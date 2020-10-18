/**
 * @file
 * vuo.image.interpretData node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include <OpenGL/CGLMacro.h>

#include "VuoData.h"

VuoModuleMetadata({
	"title": "Interpret Data as Image",
	"keywords": [
		"reinterpret", "raw", "bytes", "file reader", "file format",
		"convert", "make", "create", "visualize",
		"glitch", "corruption", "error", "broken", "garbage", "artifacts",
		"texture", "pattern",
		"databending", "datamosh",
		"snowcrash", "crash", "DOS", "CGA", "EGA", "VGA",
	],
	"version": "1.0.0",
	"dependencies": [
	],
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoData) data,
	VuoInputData(VuoInteger, {"default":256, "suggestedMin":1, "suggestedStep":32}) width,
	VuoInputData(VuoInteger, {"default":256, "suggestedMin":1, "suggestedStep":32}) height,
	VuoInputData(VuoInteger, {"menuItems":[
		{"value":0, "name":"Grayscale"},
		{"value":1, "name":"RGB"},
	], "default":1}) format,
	VuoInputData(VuoImageColorDepth, {"default":"8bpc"}) depth,
	VuoInputData(VuoBoolean, {"default":true}) repeat,
	VuoOutputData(VuoImage) image)
{
	if (!data.data || data.size < 1 || width < 1 || height < 1)
	{
		*image = NULL;
		return;
	}

	VuoInteger rowBytes = width;
	if (format == 1)
		rowBytes *= 3;

	if (depth == VuoImageColorDepth_16)
		rowBytes *= 2;
	else if (depth == VuoImageColorDepth_32)
		rowBytes *= 4;

	VuoInteger requiredBytes = rowBytes * height;

	unsigned char *pixels = calloc(1, requiredBytes);
	if (data.size >= requiredBytes)
		for (int y = 0; y < height; ++y)
			memcpy(pixels + rowBytes * (height - 1 - y), data.data + rowBytes * y, rowBytes);
	else
	{
		unsigned char *p = pixels + rowBytes * (height - 1);
		unsigned char *d = (unsigned char *)data.data;
		for (; p >= pixels; p -= rowBytes)
		{
			// If there's enough data to fill the row, just copy the whole row.
			if (d + rowBytes < (unsigned char *)data.data + data.size)
			{
				memcpy(p, d, rowBytes);
				d += rowBytes;
			}

			// If there isn't enough data to fill the row, copy the available data…
			else
			{
				size_t remainingBytes = (unsigned char *)data.data + data.size - d;
				memcpy(p, d, remainingBytes);
				if (!repeat)
					break;

				// …then start over.
				d = (unsigned char *)data.data;
				for (int x = remainingBytes; x < rowBytes; ++x)
				{
					p[x] = *(d++);
					if (d >= (unsigned char *)data.data + data.size)
						d = (unsigned char *)data.data;
				}
			}
		}
	}

	*image = VuoImage_makeFromBuffer(pixels, format == 1 ? GL_RGB : GL_LUMINANCE, width, height, depth, ^(void *pixels){
		free(pixels);
	});
}
