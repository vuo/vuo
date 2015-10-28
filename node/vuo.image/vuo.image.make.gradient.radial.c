/**
 * @file
 * vuo.image.make.gradient.radial node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Radial Gradient Image",
					  "keywords" : [ "backdrop", "background", "ramp", "interpolate", "color" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "CompareImageGenerators.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
		// VuoInputEvent({"eventBlocking":"none","data":"colors"}) colorsEvent,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedStep":0.1}) radius,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	VuoShader shader = VuoShader_makeRadialGradientShader(colors, center, radius, width, height);
	VuoRetain(shader);

	// Render.
	*image = VuoImageRenderer_draw((*instance)->imageRenderer, shader, width, height, VuoImageColorDepth_8);

	VuoRelease(shader);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
