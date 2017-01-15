/**
 * @file
 * vuo.image.color.split.rgb node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Split Image RGB Channels",
					  "keywords" : [ "separate", "colors", "filter" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "SeparateRedGreenBlue.vuo", "EnhanceBlue.vuo" ]
					  }
				 });

static const char *redShader = VUOSHADER_GLSL_SOURCE(120,
	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;

	void main(void)
	{
		vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
		gl_FragColor = vec4(color.r, color.r, color.r, color.a);
	}
);

static const char *greenShader = VUOSHADER_GLSL_SOURCE(120,
	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;

	void main(void)
	{
		vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
		gl_FragColor = vec4(color.g, color.g, color.g, color.a);
	}
);

static const char *blueShader = VUOSHADER_GLSL_SOURCE(120,
	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;

	void main(void)
	{
		vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
		gl_FragColor = vec4(color.b, color.b, color.b, color.a);
	}
);

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
	VuoShader redShader;
	VuoShader greenShader;
	VuoShader blueShader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->redShader = VuoShader_make("Split Image RGB Colors Shader (Red)");
	VuoShader_addSource(instance->redShader, VuoMesh_IndividualTriangles, NULL, NULL, redShader);
	VuoRetain(instance->redShader);

	instance->greenShader = VuoShader_make("Split Image RGB Colors Shader (Green)");
	VuoShader_addSource(instance->greenShader, VuoMesh_IndividualTriangles, NULL, NULL, greenShader);
	VuoRetain(instance->greenShader);

	instance->blueShader = VuoShader_make("Split Image RGB Colors Shader (Blue)");
	VuoShader_addSource(instance->blueShader, VuoMesh_IndividualTriangles, NULL, NULL, blueShader);
	VuoRetain(instance->blueShader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoOutputData(VuoImage) redImage,
		VuoOutputData(VuoImage) greenImage,
		VuoOutputData(VuoImage) blueImage
)
{
	if (!image)
		return;

	VuoShader_setUniform_VuoImage((*instance)->redShader, "texture", image);
	*redImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->redShader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoImage((*instance)->greenShader, "texture", image);
	*greenImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->greenShader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoShader_setUniform_VuoImage((*instance)->blueShader, "texture", image);
	*blueImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->blueShader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->redShader);
	VuoRelease((*instance)->greenShader);
	VuoRelease((*instance)->blueShader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
