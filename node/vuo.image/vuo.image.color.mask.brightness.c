/**
 * @file
 * vuo.image.color.mask.brightness node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
					  "keywords" : [ "threshold", "remove", "depth", "cut", "magic", "wand", "transparent", "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "MaskMovieByBrightness.vuo" ]
					  }
				 });

static const char * thresholdFragmentShader = VUOSHADER_GLSL_FRAGMENT_SOURCE_WITH_COLOR_CONVERSIONS(

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float threshold;
	uniform float sharpness;

	void main(void)
	{
		vec4 rgb = texture2D(texture, fragmentTextureCoordinate.xy);
		vec3 hsl = RgbToHsl(rgb.rgb);

		rgb *= smoothstep(threshold*sharpness, threshold*(2-sharpness), hsl.z);

		gl_FragColor = rgb;
	}
);

static const char * colorFragmentShader = VUOSHADER_GLSL_FRAGMENT_SOURCE_WITH_COLOR_CONVERSIONS(

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float threshold;
	uniform float sharpness;
	uniform vec4 mask;

	void main(void)
	{
		vec4 rgb = texture2D(texture, fragmentTextureCoordinate.xy);
		vec4 val = rgb*mask;

		rgb *= smoothstep(threshold*sharpness, threshold*(2-sharpness), max(max(val.x, val.y), max(val.z, val.a)));

		gl_FragColor = rgb;
	}
);

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1}) threshold,
		VuoInputData(VuoThresholdType, {"default":"luminance"}) thresholdType,
		VuoInputData(VuoReal, {"default":0.9, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoOutputData(VuoImage) maskedImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader frag;
	switch(thresholdType)
	{
		case VuoThresholdType_Luminance:
			frag = VuoShader_make("Luminance Threshold Shader", VuoShader_getDefaultVertexShader(), thresholdFragmentShader);
			break;

		case VuoThresholdType_Red:
			frag = VuoShader_make("Red Threshold Shader", VuoShader_getDefaultVertexShader(), colorFragmentShader);
			VuoShader_setUniformPoint4d(frag, (*instance)->glContext, "mask", (VuoPoint4d){ 1, 0, 0, 0 } );
			break;

		case VuoThresholdType_Green:
			frag = VuoShader_make("Green Threshold Shader", VuoShader_getDefaultVertexShader(), colorFragmentShader);
			VuoShader_setUniformPoint4d(frag, (*instance)->glContext, "mask", (VuoPoint4d){ 0, 1, 0, 0 } );
			break;

		case VuoThresholdType_Blue:
			frag = VuoShader_make("Blue Threshold Shader", VuoShader_getDefaultVertexShader(), colorFragmentShader);
			VuoShader_setUniformPoint4d(frag, (*instance)->glContext, "mask", (VuoPoint4d){ 0, 0, 1, 0 } );
			break;

		case VuoThresholdType_Alpha:
			frag = VuoShader_make("Alpha Threshold Shader", VuoShader_getDefaultVertexShader(), colorFragmentShader);
			VuoShader_setUniformPoint4d(frag, (*instance)->glContext, "mask", (VuoPoint4d){ 0, 0, 0, 1 } );
			break;
	}
	VuoRetain(frag);
	VuoShader_resetTextures(frag);
	VuoShader_addTexture(frag, (*instance)->glContext, "texture", image);
	VuoShader_setUniformFloat(frag, (*instance)->glContext, "threshold", MAX(threshold,0));
	VuoShader_setUniformFloat(frag, (*instance)->glContext, "sharpness", MAX(MIN(sharpness,1),0));

	*maskedImage = VuoImageRenderer_draw((*instance)->imageRenderer, frag, w, h);

	VuoRelease(frag);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
