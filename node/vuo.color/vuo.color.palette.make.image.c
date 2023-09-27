/**
 * @file
 * vuo.color.palette.make.image node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <OpenGL/CGLMacro.h>
#include <wuquant.h>

VuoModuleMetadata({
	"title": "Make Palette from Image",
	"keywords": [
		"alias", "contour", "edge", "banding", "raster", "gif", "quantize", "gradient", "reduce", "depth",
		"generate", "create", "representative",
		"WuQuant", "Xiaolin Wu", "RGB partition", "variance minimization", "clustering",
		"retro", "1980s", "1990s",
	],
	"version": "1.0.0",
	"dependencies": [
		"wuquant",
	],
	"node": {
		"exampleCompositions": [ "vuo-example://vuo.image/ComparePosterizeAndPalette.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoImage) image,
	VuoInputData(VuoInteger, {"default":16, "suggestedMin":1, "suggestedMax":256}) colors,
	VuoOutputData(VuoList_VuoColor) palette)
{
	if (!image || colors < 1)
	{
		*palette = NULL;
		return;
	}

	const unsigned char *buffer = VuoImage_getBuffer(image, GL_RGB);
	if (!buffer)
	{
		VUserLog("Error: Couldn't get pixels.");
		*palette = NULL;
		return;
	}

	int clampedColors = MIN(colors, 256);
	unsigned char *rgbBytePalette = calloc(3, clampedColors);
	if (!wuquant(buffer, image->pixelsWide * image->pixelsHigh, clampedColors, rgbBytePalette))
	{
		VUserLog("Error: wuquant() failed.");
		*palette = NULL;
		return;
	}

	*palette = VuoListCreateWithCount_VuoColor(clampedColors, (VuoColor){0,0,0,1});
	VuoColor *paletteData = VuoListGetData_VuoColor(*palette);
	for (int i = 0; i < clampedColors; ++i)
	{
		paletteData[i].r = rgbBytePalette[i * 3    ] / 255.;
		paletteData[i].g = rgbBytePalette[i * 3 + 1] / 255.;
		paletteData[i].b = rgbBytePalette[i * 3 + 2] / 255.;
	}
	free(rgbBytePalette);
}
