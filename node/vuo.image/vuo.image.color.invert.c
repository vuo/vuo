/**
 * @file
 * vuo.image.color.invert node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Invert Image Colors",
					 "keywords" : [ "reverse", "negative", "flip", "filter" ],
					 "version" : "1.1.2",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * invertFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;
	uniform sampler2D texture;

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate);
		color.rgb /= color.a > 0. ? color.a : 1.;
		color.rgb = 1. - color.rgb;
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
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Invert Color Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, invertFragmentShader);
	VuoRetain(instance->shader);
	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoOutputData(VuoImage) invertedImage
)
{
	if (!image)
	{
		*invertedImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	*invertedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
