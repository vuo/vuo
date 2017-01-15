/**
 * @file
 * vuo.image.make.triangle node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Triangle Image",
					  "keywords" : [ "backdrop", "serpienski", "prototype", "shape"],
					  "version" : "1.0.0",
					  "node": {
						  // "exampleCompositions" : [ ".vuo" ]
					  }
				 });

static const char * triangleFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;
	uniform vec4 color;
	uniform vec2 a;	// must be wound clockwise
	uniform vec2 b;
	uniform vec2 c;

	bool isLeft(vec2 a, vec2 b, vec2 c)
	{
		return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) > 0;
	}

	void main(void)
	{
		vec2 pos = fragmentTextureCoordinate.xy;
		gl_FragColor = !isLeft(a, b, pos) && !isLeft(b, c, pos) && !isLeft(c, a, pos) ? color : vec4(0,0,0,0);
	}
);

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

	instance->shader = VuoShader_make("Triangle Shader", VuoShader_getDefaultVertexShader(), triangleFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	VuoShader_setUniformColor((*instance)->shader, (*instance)->glContext, "color", color);

	float h = sin(45.) * width/height;
	float offset = (1-h)/2.;

	VuoShader_setUniformPoint2d((*instance)->shader, (*instance)->glContext, "a", (VuoPoint2d) { 0, offset});
	VuoShader_setUniformPoint2d((*instance)->shader, (*instance)->glContext, "b", (VuoPoint2d) { .5, h+offset});
	VuoShader_setUniformPoint2d((*instance)->shader, (*instance)->glContext, "c", (VuoPoint2d) { 1, offset });

	// Render.
	*image = VuoImageRenderer_draw((*instance)->imageRenderer, shader, width, height);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
