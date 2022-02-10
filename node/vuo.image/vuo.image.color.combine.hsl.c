/**
 * @file
 * vuo.image.color.combine.hsl node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
	"title": "Combine Image HSL Channels",
	"keywords": [
		"filter", "sum", "add",
		"alpha", "transparent",
		"tone", "chroma", "colors",
		"brightness", "luminance", "luma", "hsla",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ "CapSaturation.vuo", "ZoomBlurHue.vuo" ],
	},
});

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"
	\n#include "VuoGlslHsl.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform sampler2D hueTexture;
	uniform int hueExists;

	uniform sampler2D saturationTexture;
	uniform int saturationExists;

	uniform sampler2D lightnessTexture;
	uniform int lightnessExists;

	uniform sampler2D alphaTexture;
	uniform int alphaExists;

	void main(void)
	{
		vec4 hueColor        = VuoGlsl_sample(hueTexture,        fragmentTextureCoordinate);
		vec4 saturationColor = VuoGlsl_sample(saturationTexture, fragmentTextureCoordinate);
		vec4 lightnessColor  = VuoGlsl_sample(lightnessTexture,  fragmentTextureCoordinate);
		vec4 alphaColor      = VuoGlsl_sample(alphaTexture,      fragmentTextureCoordinate);

		vec4 result = vec4(0.);

		float hue = 0;
		if (hueExists == 1)
		{
			// Support both color and greyscale hue representations.
			hueColor.rgb /= hueColor.a;
			vec3 hueHsl = VuoGlsl_rgbToHsl(hueColor.rgb);
			if (hueHsl.y > 0.01) // saturation > 1%
				hue = hueHsl.x;  // hue
			else
				hue = (hueColor.r + hueColor.g + hueColor.b)/3.;

			result.a += hueColor.a;
		}

		float saturation = 1.;
		if (saturationExists == 1)
		{
			saturationColor.rgb /= saturationColor.a;
			saturation = (saturationColor.r + saturationColor.g + saturationColor.b)/3.;
			result.a += saturationColor.a;
		}

		float lightness = 0.5;
		if (lightnessExists == 1)
		{
			lightnessColor.rgb /= lightnessColor.a;
			lightness = (lightnessColor.r + lightnessColor.g + lightnessColor.b)/3.;
			result.a += lightnessColor.a;
		}

		result.rgb = VuoGlsl_hslToRgb(vec3(hue, saturation, lightness));

		result.a /= float(hueExists + saturationExists + lightnessExists);

		if (alphaExists == 1)
			result.a *= VuoGlsl_rgbToHsl(alphaColor.rgb).z;

		result.rgb *= result.a;

		gl_FragColor = result;
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

	instance->shader = VuoShader_make("Combine Image RGB Colors Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	// An opaque, nonwhite `opacityImage` should output a semitransparent image with an alpha channel.
	instance->shader->isTransparent = true;

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) hueImage,
		VuoInputData(VuoImage) saturationImage,
		VuoInputData(VuoImage) lightnessImage,
		VuoInputData(VuoImage) opacityImage,
		VuoOutputData(VuoImage) combinedImage
)
{
	VuoImage provokingImage = NULL;
	if (hueImage)
		provokingImage = hueImage;
	else if (saturationImage)
		provokingImage = saturationImage;
	else if (lightnessImage)
		provokingImage = lightnessImage;
	else
	{
		*combinedImage = opacityImage;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "hueTexture",   hueImage);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "hueExists",  hueImage?1:0);

	VuoShader_setUniform_VuoImage((*instance)->shader, "saturationTexture",  saturationImage);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "saturationExists", saturationImage?1:0);

	VuoShader_setUniform_VuoImage((*instance)->shader, "lightnessTexture",  lightnessImage);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "lightnessExists", lightnessImage?1:0);

	VuoShader_setUniform_VuoImage((*instance)->shader, "alphaTexture",  opacityImage);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "alphaExists", opacityImage?1:0);

	*combinedImage = VuoImageRenderer_render((*instance)->shader, provokingImage->pixelsWide, provokingImage->pixelsHigh, VuoImage_getColorDepth(provokingImage));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
