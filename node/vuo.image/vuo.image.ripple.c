/**
 * @file
 * vuo.image.ripple node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

#include "VuoDisplacement.h"

VuoModuleMetadata({
					 "title" : "Ripple Image Directionally",
					 "keywords" : [
						 "wave", "sinusoidal", "sine", "cosine", "undulate", "ruffle", "swish", "swing", "flap", "sway", "billow", "water",
						 "linear",
						 "filter"
					 ],
					 "version" : "2.2.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						  "exampleCompositions" : [ "vuo-example://vuo.scene/RippleImageOfSphere.vuo", "CompareRippleTypes.vuo" ]
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	// Inputs
	uniform sampler2D texture;
	uniform float aspectRatio;
	uniform float angle;
	uniform float angleDelta;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		float samplerPhase = dot(vec2(cos(angle), sin(angle)), (fragmentTextureCoordinate.xy - vec2(.5,.5)) * vec2(aspectRatio, 1.));
		float offset = sin(samplerPhase/wavelength + phase) * amplitude;
		gl_FragColor = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy + vec2(cos(angle+angleDelta)*offset, sin(angle+angleDelta)*offset) * vec2(1., aspectRatio));
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

	instance->shader = VuoShader_make("Ripple Image Directionally");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":135.0,"suggestedMin":0,"suggestedMax":360,"suggestedStep":1}) angle,
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

	double nonzeroWavelength = VuoReal_makeNonzero(wavelength);

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "angle", angle*M_PI/180.);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "angleDelta", displacement == VuoDisplacement_Transverse ? M_PI/2. : 0);
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
