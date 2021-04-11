/**
 * @file
 * vuo.image.make.gradient node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Linear Gradient Image",
					  "keywords" : [ "backdrop", "background", "ramp", "interpolate", "color" ],
					  "version" : "1.0.1",
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "MoveLinearGradient.vuo" ]
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

	instance->shader = VuoShader_makeLinearGradientShader();
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
		VuoInputData(VuoPoint2d, {"default":{"x":-1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) start,
		VuoInputData(VuoPoint2d,  {"default":{"x":1,"y":-1}, "suggestedStep":{"x":0.1,"y":0.1}}) end,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) noiseAmount,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	VuoShader_setLinearGradientShaderValues((*instance)->shader, colors, start, end, 1, noiseAmount);

	// Render.
	*image = VuoImageRenderer_render((*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
