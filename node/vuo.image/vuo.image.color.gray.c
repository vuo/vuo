/**
 * @file
 * vuo.image.color.grey node implementation.
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
					  "title" : "Make Grayscale Image",
					  "keywords" : [
						  "luminance", "luma", "RGB", "red", "green", "blue", "chroma", "alpha",
						  "black and white",
						  "greyscale", // international spelling
						  "lightness", "darkness",
						  "minimum", "maximum", "average", "component", "decomposition",
						  "ITU-R Recommendation BT.601 1982 NTSC CRT CCIR",
						  "ITU-R Recommendation BT.709 1990 HDTV",
						  "HSL", "desaturate",
						  "filter"
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "ColorMyWorld.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)
	include(VuoGlslBrightness)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float amount;
	uniform int brightnessType;

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy);
		color.rgb /= color.a > 0. ? color.a : 1.;

		color.rgb = mix(color.rgb, vec3(VuoGlsl_brightness(color, brightnessType)), amount);

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

	instance->shader = VuoShader_make("Grayscale");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoThresholdType, {"default":"rec709", "includeValues":["rec601","rec709","desaturate","rgb-average","rgb-minimum","rgb-maximum","red","green","blue"]}) type, // Hide "alpha" and "rgb" since they aren't relevant to this node.
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.01}) amount,
		VuoOutputData(VuoImage) grayImage
)
{
	if (!image)
	{
		*grayImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "brightnessType",    type);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "amount",  amount);

	*grayImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
