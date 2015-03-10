/**
 * @file
 * vuo.image.color.invert node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Invert Image Colors",
					 "keywords" : [ "reverse", "negative", "filter" ],
					 "version" : "1.0.0",
				 });

static const char * invertFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;

	void main(void)
	{
		vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
		float a = color.a;
		color = vec4(a - color.rgb, a);
		gl_FragColor = color;
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
		VuoOutputData(VuoImage) invertedImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;
	VuoShader frag = VuoShader_make("Invert Color Shader", VuoShader_getDefaultVertexShader(), invertFragmentShader);
	VuoRetain(frag);
	VuoShader_resetTextures(frag);
	VuoShader_addTexture(frag, (*instance)->glContext, "texture", image);
	*invertedImage = VuoImageRenderer_draw((*instance)->imageRenderer, frag, w, h);

	VuoRelease(frag);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
