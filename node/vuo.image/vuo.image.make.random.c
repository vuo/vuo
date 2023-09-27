/**
 * @file
 * vuo.image.make.random node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Random Image",
					  "keywords" : [ "noise", "snow", "static" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "MakeRandomImagePattern.vuo" ]
					  }
				 });

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslRandom.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform vec4 colorA;
	uniform vec4 colorB;
	uniform float seed;
	uniform float aspectRatio;

	void main()
	{
		vec2 noiseCoordinate = fragmentTextureCoordinate;
		noiseCoordinate.x += seed;
		float intensity = VuoGlsl_random2D1D(noiseCoordinate);
		gl_FragColor = mix(colorA, colorB, intensity);
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

	instance->shader = VuoShader_make("Random Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) colorA,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) colorB,
		VuoInputData(VuoInteger) seed,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	VuoShader_setUniform_VuoColor((*instance)->shader, "colorA", colorA);
	VuoShader_setUniform_VuoColor((*instance)->shader, "colorB", colorB);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "seed", 2.*seed);

	// Render.
	*image = VuoImageRenderer_render((*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
