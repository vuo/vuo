/**
 * @file
 * vuo.image.color.split.rgb node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Split Image RGB Channels",
					  "keywords" : [ "separate", "colors", "filter", "alpha" ],
					  "version" : "1.1.1",
					  "node" : {
						  "exampleCompositions" : [ "SeparateRedGreenBlue.vuo", "EnhanceBlue.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec4 colorMask;
	uniform bool alpha;	// false = use texture's alpha; true = 100%

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy);
		if (alpha && color.a > 0.)
			color.rgb /= color.a;
		float luminance = dot(color, colorMask);
		gl_FragColor = vec4(luminance, luminance, luminance, alpha ? 1. : color.a);
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

	instance->shader = VuoShader_make("Split Image RGB Colors Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoBoolean, {"default":true}) preserveOpacity,
		VuoOutputData(VuoImage) redImage,
		VuoOutputData(VuoImage) greenImage,
		VuoOutputData(VuoImage) blueImage,
		VuoOutputData(VuoImage) opacityImage
)
{
	if (!image)
	{
		*redImage = *greenImage = *blueImage = *opacityImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){1,0,0,0});
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "alpha", !preserveOpacity);
	*redImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,1,0,0});
	*greenImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,0,1,0});
	*blueImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "colorMask", (VuoPoint4d){0,0,0,1});
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "alpha", true);
	*opacityImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
