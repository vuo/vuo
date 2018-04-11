/**
 * @file
 * vuo.image.color.mask.brightness node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoShader.h"
#include "VuoPoint3d.h"
#include "VuoThresholdType.h"

VuoModuleMetadata({
					  "title" : "Mask Image by Brightness",
					  "keywords" : [
						  "threshold", "remove", "cut", "keying", "depth", "transparent",
						  "luminance", "luma", "red", "green", "blue", "chroma", "alpha",
						  "magic", "wand",
						  "filter"
					  ],
					  "version" : "1.2.0",
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "MaskMovieByBrightness.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)
	include(VuoGlslBrightness)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float threshold;
	uniform float sharpness;
	uniform int brightnessType;

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy);
		color.rgb /= color.a > 0. ? color.a : 1.;

		color *= smoothstep(threshold*sharpness, threshold*(2-sharpness), VuoGlsl_brightness(color, brightnessType));
		color.rgb *= color.a;

		gl_FragColor = color;
	}
);

struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Mask Image by Brightness");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1}) threshold,
		VuoInputData(VuoThresholdType, {"default":"rec709", "includeValues":["rec601","rec709","desaturate","rgb-average","rgb-minimum","rgb-maximum","red","green","blue","alpha"]}) thresholdType, // Hide "rgb" since it isn't relevant to this node.
		VuoInputData(VuoReal, {"default":0.9, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoOutputData(VuoImage) maskedImage
)
{
	if (!image)
	{
		*maskedImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture",   image);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "brightnessType", thresholdType);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "threshold", MAX(threshold,0));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "sharpness", MAX(MIN(sharpness,1),0));

	*maskedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
