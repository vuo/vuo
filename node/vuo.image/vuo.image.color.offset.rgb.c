/**
 * @file
 * vuo.image.color.offset.rgb node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Offset Image RGB Channels",
					 "keywords" : [ "red", "green", "blue", "colors",
						 "separate", "move",
						 "prism", "chromatic", "aberration", "shift",
						 "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoImageRenderer"
					 ],
					 "node" : {
						 "exampleCompositions" : [ "OffsetColors.vuo" ]
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform vec2 redOffset;
	uniform vec2 greenOffset;
	uniform vec2 blueOffset;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec4 redColor   = texture2D(texture, fragmentTextureCoordinate.xy - redOffset   + vec2(0.5,0.5));
		vec4 greenColor = texture2D(texture, fragmentTextureCoordinate.xy - greenOffset + vec2(0.5,0.5));
		vec4 blueColor  = texture2D(texture, fragmentTextureCoordinate.xy - blueOffset  + vec2(0.5,0.5));

		gl_FragColor = vec4(redColor.r, greenColor.g, blueColor.b, (redColor.a + greenColor.a + blueColor.a)/3.);
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

	instance->shader = VuoShader_make("Offset Image RGB Colors Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
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
		VuoInputData(VuoPoint2d, {"default":{"x":0.01,"y":0.0},    "suggestedMin":{"x":-1.0,"y":-1.0}, "suggestedMax":{"x":1.0,"y":1.0}}) redOffset,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.01},    "suggestedMin":{"x":-1.0,"y":-1.0}, "suggestedMax":{"x":1.0,"y":1.0}}) greenOffset,
		VuoInputData(VuoPoint2d, {"default":{"x":-0.01,"y":-0.01}, "suggestedMin":{"x":-1.0,"y":-1.0}, "suggestedMax":{"x":1.0,"y":1.0}}) blueOffset,
		VuoOutputData(VuoImage) offsetImage
)
{
	if (! image)
		return;

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture",     image);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "redOffset",   VuoShader_samplerCoordinatesFromVuoCoordinates(redOffset,   image));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "greenOffset", VuoShader_samplerCoordinatesFromVuoCoordinates(greenOffset, image));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "blueOffset",  VuoShader_samplerCoordinatesFromVuoCoordinates(blueOffset,  image));

	*offsetImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
