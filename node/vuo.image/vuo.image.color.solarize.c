/**
 * @file
 * vuo.image.color.solarize node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Solarize Image",
					 "keywords" : [
						 "invert", "reverse", "negative",
						 "brightness", "lightness", "luminance",
						 "Sabattier", "darkroom",
						 "filter"
					  ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;
	uniform sampler2D image;

	uniform float invert;
	uniform float threshold;
	uniform float smoothness;

	const vec3 W = vec3(0.2125, 0.7154, 0.0721);

	void main(void)
	{
		vec4 color = VuoGlsl_sample(image, fragmentTextureCoordinate);
		float luminance = dot(color.rgb, W);
		float thresholdResult = invert + (1. - invert*2.) * smoothstep(luminance-smoothness, luminance+smoothness, threshold);
		gl_FragColor = vec4(mix(color.rgb, 1. - color.rgb, thresholdResult), color.a);
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

	instance->shader = VuoShader_make("Solarize Image");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) threshold,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1}) sharpness,
	VuoInputData(VuoBoolean, {"default":true}) invertLighterColors,
	VuoOutputData(VuoImage) solarizedImage
)
{
	if(!image)
	{
		*solarizedImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "image", image);
	VuoShader_setUniform_VuoReal((*instance)->shader, "invert", invertLighterColors ? 1 : 0);
	VuoShader_setUniform_VuoReal((*instance)->shader, "threshold", threshold * 256./255.);
	VuoShader_setUniform_VuoReal((*instance)->shader, "smoothness", (1. - MIN(1., sharpness)) / 2.);

	*solarizedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
