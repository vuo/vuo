/**
 * @file
 * vuo.image.color.split.hsl node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Split Image HSL Channels",
					  "keywords" : [ "separate", "colors", "filter", "alpha" ],
					  "version" : "1.0.1",
					  "node" : {
						  "exampleCompositions" : [ "SeparateHueSaturationLightness.vuo", "CapSaturation.vuo", "ZoomBlurHue.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)
	include(hsl)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec4 colorMask;
	uniform bool alpha;	// false = use texture's alpha; true = 100%
	uniform bool colorHueImage;	// false = use greyscale hue; true = use color hue

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy);
		color.rgb /= color.a > 0. ? color.a : 1.;
		vec4 hsla = vec4(rgbToHsl(color.rgb), color.a);

		vec4 rgba;
		if (colorHueImage)
			rgba = vec4(hslToRgb(vec3(hsla.x, 1., .5)), alpha ? 1. : color.a);
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
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

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
	*hueImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,1,0,0});
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "colorHueImage", false);
	*saturationImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,0,1,0});
	*lightnessImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,0,0,1});
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "alpha", true);
	*opacityImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
