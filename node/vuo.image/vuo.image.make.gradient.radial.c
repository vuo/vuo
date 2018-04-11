/**
 * @file
 * vuo.image.make.gradient.radial node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Radial Gradient Image",
					  "keywords" : [ "backdrop", "background", "ramp", "interpolate", "color",
									 "circle", "oval", "ellipse", "rounded" ],
					  "version" : "1.0.2",
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "MoveRadialGradient.vuo" ]
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

	instance->shader = VuoShader_makeRadialGradientShader();
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) radius,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) noiseAmount,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	VuoShader_setRadialGradientShaderValues((*instance)->shader, colors, center, radius, width, height, noiseAmount);

	// Render.
	*image = VuoImageRenderer_render((*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
