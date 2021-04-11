/**
 * @file
 * vuo.image.filmGrain node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

#include "VuoDisplacement.h"

VuoModuleMetadata({
	"title": "Add Film Grain",
	"keywords": [
		"noise", "random", "particles", "speckles", "freckles", "spots",
		"photographic film stock", "granularity", "ISO",
		"old-fashioned", "vintage",
		"filter"
	],
	"version": "1.0.0",
	"dependencies": [
		"VuoImageRenderer"
	],
	"node": {
		"exampleCompositions": [ ]
	}
});


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"
	\n#include "noise3D.glsl"

	// Inputs
	uniform sampler2D texture;
	uniform float aspectRatio;
	uniform float time;
	uniform float density;
	uniform float scale;
	uniform float amount;
	varying vec2 fragmentTextureCoordinate;

	float BlendSoftLightf(float base, float blend) { return ((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend))); }

	void main()
	{
		vec4 backgroundColor = VuoGlsl_sample(texture, fragmentTextureCoordinate);
		backgroundColor.rgb /= backgroundColor.a > 0. ? backgroundColor.a : 1.;

		vec2 p = fragmentTextureCoordinate;
		p -= .5;
		float n = snoise3D1D(vec3(p.x * scale, p.y * scale / aspectRatio, time));
		n = (clamp(n, density, 1.) - density) / (1. - density);

		// Preserve average brightness.
		n += .25 + density/4.;

		vec3 blendedColors = clamp(vec3(BlendSoftLightf(backgroundColor.r, n),
										BlendSoftLightf(backgroundColor.g, n),
										BlendSoftLightf(backgroundColor.b, n)), 0., 1.);

		gl_FragColor = vec4(blendedColors * backgroundColor.a * amount + backgroundColor.rgb * backgroundColor.a * (1. - amount), backgroundColor.a);
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

	instance->shader = VuoShader_make("Film Grain");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoReal) time,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0,"suggestedMax":1}) density,
	VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.001, "suggestedMax":0.02, "suggestedStep":0.001}) scale,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) amount,
	VuoOutputData(VuoImage) grainedImage
)
{
	if (!image)
	{
		*grainedImage = NULL;
		return;
	}

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "time", time * 60.);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "density", 1. - VuoReal_clamp(density, 0.001, 1.) * 2.);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "scale", 1./VuoReal_makeNonzero(scale));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "amount", amount);

	// Render.
	*grainedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
