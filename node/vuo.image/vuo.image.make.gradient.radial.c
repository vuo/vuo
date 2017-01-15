/**
 * @file
 * vuo.image.make.gradient.radial node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "MoveRadialGradient.vuo" ]
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

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	VuoList_VuoColor colors = VuoListCreate_VuoColor();
	VuoRetain(colors);

	instance->shader = VuoShader_makeRadialGradientShader(colors, VuoPoint2d_make(0., 0.), 1., 1., 1., 0.);
	VuoRetain(instance->shader);

	VuoRelease(colors);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
		// VuoInputEvent({"eventBlocking":"none","data":"colors"}) colorsEvent,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedStep":0.1}) radius,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) noiseAmount,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	// VuoPoint2d scale = width < height ? VuoPoint2d_make(1., height/(float)width) : VuoPoint2d_make(width/(float)height, 1.);
	VuoPoint2d scale = VuoPoint2d_make(1., height/(float)width);

	int len = VuoListGetCount_VuoColor(colors);
	unsigned char* pixels = (unsigned char*)malloc(sizeof(char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValue_VuoColor(colors, i);
		pixels[n++] = (unsigned int)(col.a*col.b*255);
		pixels[n++] = (unsigned int)(col.a*col.g*255);
		pixels[n++] = (unsigned int)(col.a*col.r*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}

	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_BGRA, len, 1, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "gradientStrip", gradientStrip);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "gradientCount", len);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoPoint2d_make((center.x+1)/2, (center.y+1)/2));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "radius", radius > 0. ? radius/2. : 0);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "scale",  VuoPoint2d_make(scale.x, scale.y));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "noiseAmount", MAX(0.,noiseAmount/10.));

	// Render.
	*image = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
