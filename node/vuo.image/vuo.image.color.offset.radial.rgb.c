/**
 * @file
 * vuo.image.color.offset.radial.rgb node implementation.
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
					 "title" : "Radially Offset Image RGB Channels",
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
						 "exampleCompositions" : [ "FlyAtWarpSpeed.vuo" ]
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform float redOffset;
	uniform float greenOffset;
	uniform float blueOffset;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec4 redColor   = texture2D(texture, (fragmentTextureCoordinate.xy - vec2(.5,.5)) * redOffset   + vec2(0.5,0.5));
		vec4 greenColor = texture2D(texture, (fragmentTextureCoordinate.xy - vec2(.5,.5)) * greenOffset + vec2(0.5,0.5));
		vec4 blueColor  = texture2D(texture, (fragmentTextureCoordinate.xy - vec2(.5,.5)) * blueOffset  + vec2(0.5,0.5));

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
		VuoInputData(VuoReal, {"default":-0.1, "suggestedMin":-1.0, "suggestedMax":1.0}) redOffset,
		VuoInputData(VuoReal, {"default": 0.0, "suggestedMin":-1.0, "suggestedMax":1.0}) greenOffset,
		VuoInputData(VuoReal, {"default": 0.1, "suggestedMin":-1.0, "suggestedMax":1.0}) blueOffset,
		VuoOutputData(VuoImage) offsetImage
)
{
	if (! image)
		return;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture",     image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "redOffset",   1/(redOffset  +1));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "greenOffset", 1/(greenOffset+1));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "blueOffset",  1/(blueOffset +1));

	*offsetImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
