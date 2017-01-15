/**
 * @file
 * vuo.image.color.mask.brightness node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
					  "version" : "1.1.1",
					  "node": {
						  "exampleCompositions" : [ "MaskMovieByBrightness.vuo" ]
					  }
				 });

static const char *thresholdFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(hsl)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float threshold;
	uniform float sharpness;

	void main(void)
	{
		vec4 rgb = texture2D(texture, fragmentTextureCoordinate.xy);
		vec3 hsl = rgbToHsl(rgb.rgb);

		rgb *= smoothstep(threshold*sharpness, threshold*(2-sharpness), hsl.z);

		gl_FragColor = rgb;
	}
);

static const char *colorFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(hsl)

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
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
	int thresholdTypeCurrent;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->thresholdTypeCurrent = -1;

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

	if( (int)thresholdType != (*instance)->thresholdTypeCurrent )
	{
		if( (*instance)->shader != NULL)
			VuoRelease( (*instance)->shader );

		switch(thresholdType)
		{
			case VuoThresholdType_Luminance:
				(*instance)->shader = VuoShader_make("Threshold Shader (Luminance)");
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, thresholdFragmentShader);
				break;

			case VuoThresholdType_Red:
				(*instance)->shader = VuoShader_make("Threshold Shader (Red)");
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, colorFragmentShader);
				VuoShader_setUniform_VuoPoint4d((*instance)->shader, "mask", (VuoPoint4d){ 1, 0, 0, 0 } );
				break;

			case VuoThresholdType_Green:
				(*instance)->shader = VuoShader_make("Threshold Shader (Green)");
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, colorFragmentShader);
				VuoShader_setUniform_VuoPoint4d((*instance)->shader, "mask", (VuoPoint4d){ 0, 1, 0, 0 } );
				break;

			case VuoThresholdType_Blue:
				(*instance)->shader = VuoShader_make("Threshold Shader (Blue)");
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, colorFragmentShader);
				VuoShader_setUniform_VuoPoint4d((*instance)->shader, "mask", (VuoPoint4d){ 0, 0, 1, 0 } );
				break;

			case VuoThresholdType_Alpha:
				(*instance)->shader = VuoShader_make("Threshold Shader (Alpha)");
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, colorFragmentShader);
				VuoShader_setUniform_VuoPoint4d((*instance)->shader, "mask", (VuoPoint4d){ 0, 0, 0, 1 } );
				break;
		}

		(*instance)->thresholdTypeCurrent = (int)thresholdType;
		VuoRetain( (*instance)->shader );
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture",   image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "threshold", MAX(threshold,0));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "sharpness", MAX(MIN(sharpness,1),0));

	*maskedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	if((*instance)->shader != NULL)
		VuoRelease((*instance)->shader);

	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
