/**
 * @file
 * vuo.image.make.gradient node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Linear Gradient Image",
					  "keywords" : [ "backdrop", "background", "ramp", "interpolate", "color" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "MoveLinearGradient.vuo" ]
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

	instance->shader = VuoShader_makeLinearGradientShader(colors, (VuoPoint2d){0,0}, (VuoPoint2d){0,0}, 0);
	VuoRetain(instance->shader);

	VuoRelease(colors);

	return instance;
}

VuoImage makeGradientStrip(VuoList_VuoColor colors)
{
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

	return VuoImage_makeFromBuffer(pixels, GL_BGRA, len, 1, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });
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
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "start", VuoPoint2d_make((start.x+1)/2, (start.y+1)/2));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "end", VuoPoint2d_make((end.x+1)/2, (end.y+1)/2));

	VuoImage gradientStrip = makeGradientStrip(colors);

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "gradientStrip", gradientStrip);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "gradientCount", VuoListGetCount_VuoColor(colors));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "noiseAmount",   MAX(0.,noiseAmount/10.));

	// Render.
	*image = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
