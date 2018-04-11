/**
 * @file
 * vuo.image.mask.brightness node implementation.
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
						  "lightness", "darkness",
						  "magic", "wand", "green screen", "greenscreen",
						  "filter"
					  ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "MaskMovieByBrightness.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)
	include(VuoGlslBrightness)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec2 threshold;
	uniform float blur;
	uniform bool showImage;
	uniform int brightnessType;

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy);
		color.rgb /= color.a > 0. ? color.a : 1.;

		vec4 gray = VuoGlsl_gray(color, brightnessType);

		if (!showImage)
			color.rgba = vec4(1.);

		color.r *= smoothstep(threshold.x - blur, threshold.x + blur, gray.r)
				*  smoothstep(threshold.y + blur, threshold.y - blur, gray.r);
		color.g *= smoothstep(threshold.x - blur, threshold.x + blur, gray.g)
				*  smoothstep(threshold.y + blur, threshold.y - blur, gray.g);
		color.b *= smoothstep(threshold.x - blur, threshold.x + blur, gray.b)
				*  smoothstep(threshold.y + blur, threshold.y - blur, gray.b);
//		color.rgb *= color.a;

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
		VuoInputData(VuoRange, {"default":{"minimum":0.5,"maximum":1.0},
								"requireMin":true,
								"requireMax":true,
								"suggestedMin":{"minimum":0.0,"maximum":0.0},
								"suggestedMax":{"minimum":1.0,"maximum":1.0},
								"suggestedStep":{"minimum":0.01,"maximum":0.01}}) preservedBrightnesses,
		VuoInputData(VuoReal, {"default":0.9, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoInputData(VuoThresholdType, {"default":"rec709"}) brightnessType,
		VuoInputData(VuoBoolean, {"default":true}) showImage,
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
	VuoShader_setUniform_VuoInteger((*instance)->shader, "brightnessType", brightnessType);
	VuoRange or = VuoRange_makeNonzero(VuoRange_getOrderedRange(preservedBrightnesses));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "threshold", (VuoPoint2d){ or.minimum, or.maximum });
	VuoShader_setUniform_VuoReal((*instance)->shader, "blur", VuoReal_makeNonzero((1. - sharpness)/2.));
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "showImage", showImage);

	*maskedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
