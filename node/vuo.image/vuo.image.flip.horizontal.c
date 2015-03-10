/**
 * @file
 * vuo.image.flip.horizontal node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Flip Image Horizontally",
					  "keywords" : [ "mirror", "rotate" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "FlipMovie.vuo" ]
					  }
				 });

static const char * horizontalFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;

	void main(void)
	{
		gl_FragColor = texture2D(texture, vec2(1-fragmentTextureCoordinate.x, fragmentTextureCoordinate.y));
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
		VuoOutputData(VuoImage) flippedImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;
	VuoShader frag = VuoShader_make("Horizontally Flipped Shader", VuoShader_getDefaultVertexShader(), horizontalFragmentShader);
	VuoRetain(frag);
	VuoShader_resetTextures(frag);
	VuoShader_addTexture(frag, (*instance)->glContext, "texture", image);
	*flippedImage = VuoImageRenderer_draw((*instance)->imageRenderer, frag, w, h);

	VuoRelease(frag);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
