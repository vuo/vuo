/**
 * @file
 * vuo.image.make.noise node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Noise Image",
					  "keywords" : [ "perlin", "gradient" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "ShowNoiseImage.vuo" ]
					  }
				 });

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(noise3D)

	varying vec4 fragmentTextureCoordinate;

	uniform vec4 colorA;
	uniform vec4 colorB;
	uniform vec3 center;
	uniform float scale;
	uniform float aspectRatio;

	void main()
	{
		vec3 noiseCoordinate = vec3(fragmentTextureCoordinate.x - .5, (fragmentTextureCoordinate.y - .5) / aspectRatio, 0.);
		noiseCoordinate *= scale;
		noiseCoordinate -= center;
		float intensity = snoise3D1D(noiseCoordinate) / 2. + .5;
		gl_FragColor = mix(colorA, colorB, intensity);
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

	instance->shader = VuoShader_make("Noise Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) colorA,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) colorB,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-10.0,"y":-10.0}, "suggestedMax":{"x":10.0,"y":10.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal) time,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0.001, "suggestedMax":1., "suggestedStep":0.1}) scale,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "colorA", colorA);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "colorB", colorB);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center", VuoPoint3d_make(center.x/2., center.y/2., time));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale",  1./VuoReal_makeNonzero(scale));

	// Render.
	*image = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
