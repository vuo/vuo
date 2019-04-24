/**
 * @file
 * vuo.image.flip.vertical node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Flip Image Vertically",
					  "keywords" : [ "mirror", "rotate" ],
					  "version" : "1.1.2",
					  "node": {
						  "exampleCompositions" : [ "FlipMovie.vuo" ]
					  }
				 });

static const char * verticalFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;

	void main(void)
	{
		gl_FragColor = VuoGlsl_sample(texture, vec2(fragmentTextureCoordinate.x, 1-fragmentTextureCoordinate.y));
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

	instance->shader = VuoShader_make("Vertical Flip Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, verticalFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoOutputData(VuoImage) flippedImage
)
{
	if (!image)
	{
		*flippedImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	*flippedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
