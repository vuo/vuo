/**
 * @file
 * vuo.image.color.offset.radial.rgb node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Radially Offset Image RGB Channels",
					 "keywords" : [ "red", "green", "blue", "colors",
						 "separate", "move",
						 "prism", "chromatic", "aberration", "shift",
						 "filter" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node" : {
						 "exampleCompositions" : [ "FlyAtWarpSpeed.vuo" ]
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	// Inputs
	uniform sampler2D texture;
	uniform float redOffset;
	uniform float greenOffset;
	uniform float blueOffset;
	varying vec2 fragmentTextureCoordinate;

	void main()
	{
		vec4 redColor   = VuoGlsl_sample(texture, (fragmentTextureCoordinate - vec2(.5,.5)) * redOffset   + vec2(0.5,0.5));
		vec4 greenColor = VuoGlsl_sample(texture, (fragmentTextureCoordinate - vec2(.5,.5)) * greenOffset + vec2(0.5,0.5));
		vec4 blueColor  = VuoGlsl_sample(texture, (fragmentTextureCoordinate - vec2(.5,.5)) * blueOffset  + vec2(0.5,0.5));

		gl_FragColor = vec4(redColor.r, greenColor.g, blueColor.b, (redColor.a + greenColor.a + blueColor.a)/3.);
	}
);


struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Offset Image RGB Colors Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

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
	{
		*offsetImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture",     image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "redOffset",   1/(redOffset  +1));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "greenOffset", 1/(greenOffset+1));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "blueOffset",  1/(blueOffset +1));

	*offsetImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
