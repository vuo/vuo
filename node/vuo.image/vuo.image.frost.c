/**
 * @file
 * vuo.image.frost node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Frost Image",
					  "keywords" : [ "filter", "texture", "blur", "bend", "tint", "refraction", "diffraction" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoGlContext",
						  "VuoImageRenderer"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ShowFrostNoise.vuo" ]
					  }
				  });


struct nodeInstanceData
{
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_makeFrostedGlassShader();
	VuoRetain(instance->shader);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoColor, {"default":{"r":0.88,"g":0.93,"b":1.,"a":1.}}) color,
	VuoInputData(VuoReal, {"default":1.5, "suggestedMin":0., "suggestedMax":2., "suggestedStep":0.1}) brightness,
	VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) noiseTime,
	VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseAmount,
	VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseScale,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) chromaticAberration,
	VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":50}) iterations,
	VuoOutputData(VuoImage) frostedImage
)
{
	if (! image)
		return;

	VuoShader_setFrostedGlassShaderValues((*instance)->shader, color, brightness, noiseTime, noiseAmount, noiseScale, chromaticAberration, iterations);
	VuoShader_setUniform_VuoImage((*instance)->shader, "colorBuffer", image);

	*frostedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
