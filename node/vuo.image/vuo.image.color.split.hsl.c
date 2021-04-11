/**
 * @file
 * vuo.image.color.split.hsl node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Split Image HSL Channels",
 					  "keywords" : [
						  "filter", "separate",
 						  "alpha", "transparent",
 						  "tone", "chroma", "colors",
 						  "brightness", "luminance", "luma",
 					  ],
					  "version" : "1.0.1",
					  "node" : {
						  "exampleCompositions" : [ "SeparateHueSaturationLightness.vuo", "CapSaturation.vuo", "ZoomBlurHue.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"
	\n#include "VuoGlslHsl.glsl"

	varying vec2 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec4 colorMask;
	uniform bool alpha;	// false = use texture's alpha; true = 100%
	uniform bool colorHueImage;	// false = use greyscale hue; true = use color hue

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate);
		color.rgb /= color.a > 0. ? color.a : 1.;
		vec4 hsla = vec4(VuoGlsl_rgbToHsl(color.rgb), color.a);

		vec4 rgba;
		if (colorHueImage)
			rgba = vec4(VuoGlsl_hslToRgb(vec3(hsla.x, 1., .5)), alpha ? 1. : color.a);
		else
		{
			float luminance = dot(hsla, colorMask);
			rgba = vec4(luminance, luminance, luminance, alpha ? 1. : color.a);
		}

		rgba.rgb *= rgba.a;
		gl_FragColor = rgba;
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

	instance->shader = VuoShader_make("Split Image HSL Colors Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoBoolean, {"default":false}) colorHueImage,
		VuoInputData(VuoBoolean, {"default":true}) preserveOpacity,
		VuoOutputData(VuoImage) hueImage,
		VuoOutputData(VuoImage) saturationImage,
		VuoOutputData(VuoImage) lightnessImage,
		VuoOutputData(VuoImage) opacityImage
)
{
	if (!image)
	{
		*hueImage = *saturationImage = *lightnessImage = *opacityImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){1,0,0,0});
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "alpha", !preserveOpacity);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "colorHueImage", colorHueImage);
	*hueImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,1,0,0});
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "colorHueImage", false);
	*saturationImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,0,1,0});
	*lightnessImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,0,0,1});
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "alpha", true);
	*opacityImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
