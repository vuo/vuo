/**
 * @file
 * vuo.image.frost node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Frost Image",
					  "keywords" : [
						  "filter", "texture",
						  "perlin", "simplex", "gradient",
						  "blur", "bend", "tint", "refraction", "diffraction",
					  ],
					  "version" : "1.1.0",
					  "dependencies" : [
						  "VuoImageRenderer"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ShowFrostNoise.vuo", "MakeDriftingClouds.vuo" ]
					  }
				  });


struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_makeFrostedGlassShader();
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoColor, {"default":{"r":0.88,"g":0.93,"b":1.,"a":1.}}) color,
	VuoInputData(VuoReal, {"default":1.5, "suggestedMin":0., "suggestedMax":2., "suggestedStep":0.1}) brightness,
	VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) noiseCenter,
	VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) noiseTime,
	VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseAmount,
	VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseScale,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) chromaticAberration,
	VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":4}) levels,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) roughness,
	VuoInputData(VuoReal, {"default":2.0, "suggestedMin":1.0, "suggestedMax":5.0, "suggestedStep":0.1}) spacing,
	VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":8}) iterations,
	VuoOutputData(VuoImage) frostedImage
)
{
	if (! image)
	{
		*frostedImage = NULL;
		return;
	}

	VuoShader_setFrostedGlassShaderValues((*instance)->shader, color, brightness, noiseCenter, noiseTime, noiseAmount, noiseScale, chromaticAberration, levels, roughness, spacing, iterations, ((float)image->pixelsWide)/image->pixelsHigh);
	VuoShader_setUniform_VuoImage((*instance)->shader, "colorBuffer", image);

	*frostedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
