/**
 * @file
 * vuo.image.ripple node implementation.
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
					 "title" : "Ripple Image",
					 "keywords" : [ "wave", "sinusoidal", "sine", "cosine", "undulate", "ruffle", "swish", "swing", "flap", "sway", "billow", "water", "filter" ],
					 "version" : "2.1.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoImageRenderer"
					 ],
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform float angle;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		float samplerPhase = cos(angle)*fragmentTextureCoordinate.x + sin(angle)*fragmentTextureCoordinate.y;
		float offset = sin(samplerPhase/wavelength + phase) * amplitude;
		gl_FragColor = texture2D(texture, fragmentTextureCoordinate.xy + vec2(cos(angle)*offset,sin(angle)*offset));
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

	instance->shader = VuoShader_make("Ripple Image Shader");
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
		VuoInputData(VuoReal, {"default":135.0,"suggestedMin":0,"suggestedMax":360,"suggestedStep":1}) angle,
		VuoInputData(VuoReal, {"default":0.1,"suggestedMin":0,"suggestedMax":1}) amplitude,
		VuoInputData(VuoReal, {"default":0.05,"suggestedMin":0.000001,"suggestedMax":0.05}) wavelength,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) phase,
		VuoOutputData(VuoImage) rippledImage
)
{
	if (! image)
		return;

	double nonzeroWavelength = VuoReal_makeNonzero(wavelength);

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "angle", angle*M_PI/180.);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "amplitude", amplitude);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "wavelength", nonzeroWavelength*M_PI*2.);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "phase", phase*M_PI*2.);

	// Render.
	*rippledImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
