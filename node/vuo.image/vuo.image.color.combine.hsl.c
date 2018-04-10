/**
 * @file
 * vuo.image.color.combine.hsl node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Combine Image HSL Channels",
					  "keywords" : [ "sum", "add", "colors", "filter", "alpha" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "CapSaturation.vuo", "ZoomBlurHue.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)
	include(hsl)

	varying vec4 fragmentTextureCoordinate;

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
		vec4 hueColor        = VuoGlsl_sample(hueTexture,        fragmentTextureCoordinate.xy);
		vec4 saturationColor = VuoGlsl_sample(saturationTexture, fragmentTextureCoordinate.xy);
		vec4 lightnessColor  = VuoGlsl_sample(lightnessTexture,  fragmentTextureCoordinate.xy);
		vec4 alphaColor      = VuoGlsl_sample(alphaTexture,      fragmentTextureCoordinate.xy);

		vec4 result = vec4(0.);

		float hue = 0;
		if (hueExists == 1)
		{
			// Support both color and greyscale hue representations.
			hueColor.rgb /= hueColor.a;
			vec3 hueHsl = rgbToHsl(hueColor.rgb);
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

		result.rgb = hslToRgb(vec3(hue, saturation, lightness));

		result.a /= float(hueExists + saturationExists + lightnessExists);

		if (alphaExists == 1)
			result.a *= rgbToHsl(alphaColor.rgb).z;

		result.rgb *= result.a;

		gl_FragColor = result;
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

	instance->shader = VuoShader_make("Combine Image RGB Colors Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

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

	*combinedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, provokingImage->pixelsWide, provokingImage->pixelsHigh, VuoImage_getColorDepth(provokingImage));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
