/**
 * @file
 * vuo.image.ripple.radial node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

#include "VuoDisplacement.h"

VuoModuleMetadata({
					 "title" : "Ripple Image Radially",
					 "keywords" : [
						 "wave", "sinusoidal", "sine", "cosine", "undulate", "ruffle", "swish", "swing", "flap", "sway", "billow", "water",
						 "circle", "circular", "polar",
						 "filter"
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						  "exampleCompositions" : [ "CompareRippleTypes.vuo" ]
					 }
				 });

static const char * longitudinalFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	// Inputs
	uniform sampler2D texture;
	uniform float aspectRatio;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec2 tc = fragmentTextureCoordinate.xy - vec2(.5,.5);
		float samplerPhase = length(tc * vec2(aspectRatio, 1.));
		float offset = sin(samplerPhase/wavelength - phase) * amplitude;
		gl_FragColor = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy + offset * tc);
	}
);

static const char * transverseFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	// Inputs
	uniform sampler2D texture;
	uniform float aspectRatio;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec2 aspect = vec2(aspectRatio, 1.);
		vec2 tc = fragmentTextureCoordinate.xy - vec2(.5,.5);
		tc *= aspect;
		float samplerPhase = length(tc);
		float offset = sin(samplerPhase/wavelength - phase) * amplitude;

		float r = length(tc);
		float theta = atan(tc.y, tc.x) + offset;

		float x = r * cos(theta);
		float y = r * sin(theta);

		gl_FragColor = VuoGlsl_sample(texture, vec2(x,y)/aspect + vec2(.5,.5));
	}
);

struct nodeInstanceData
{
	VuoDisplacement displacement;
	VuoShader shader;

};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->displacement = VuoDisplacement_Longitudinal;
	instance->shader = VuoShader_make("Ripple Image Radially (Longitudinal)");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, longitudinalFragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.1,"suggestedMin":0,"suggestedMax":1}) amplitude,
		VuoInputData(VuoReal, {"default":0.05,"suggestedMin":0.000001,"suggestedMax":0.05}) wavelength,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) phase,
		VuoInputData(VuoDisplacement, {"default":"longitudinal"}) displacement,
		VuoOutputData(VuoImage) rippledImage
)
{
	if (! image)
	{
		*rippledImage = NULL;
		return;
	}

	if ((*instance)->displacement != displacement)
	{
		// Switch shaders.
		VuoRelease((*instance)->shader);

		if (displacement == VuoDisplacement_Longitudinal)
		{
			(*instance)->shader = VuoShader_make("Ripple Image Radially (Longitudinal)");
			VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, longitudinalFragmentShaderSource);
		}
		else
		{
			(*instance)->shader = VuoShader_make("Ripple Image Radially (Transverse)");
			VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, transverseFragmentShaderSource);
		}

		(*instance)->displacement = displacement;
		VuoRetain((*instance)->shader);
	}

	double nonzeroWavelength = VuoReal_makeNonzero(wavelength);

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "amplitude", amplitude);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "wavelength", nonzeroWavelength*M_PI*2.);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "phase", phase*M_PI*2.);

	// Render.
	*rippledImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
