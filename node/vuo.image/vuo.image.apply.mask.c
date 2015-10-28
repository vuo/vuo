/**
 * @file
 * vuo.image.apply.mask node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Apply Mask",
					  "keywords" : [ "transparency", "negative", "remove", "cut", "magic", "wand" ],
					  "version" : "1.1.0",
					  "node": {
						  "exampleCompositions" : [ "MaskMovieWithStar.vuo" ]
					  }
				 });

static const char *maskFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(hsl)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform sampler2D mask;

	void main(void)
	{
		vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
		vec4 maskColor = texture2D(mask, fragmentTextureCoordinate.xy);
		float maskAmount = maskColor.a * rgbToHsl(maskColor.rgb).z;
		color.rgb *= maskAmount;
		color.a = maskAmount;
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
		VuoInputData(VuoImage) mask,
		VuoOutputData(VuoImage) maskedImage
)
{
	if (!image || !mask)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;
	VuoShader frag = VuoShader_make("Apply Mask");
	VuoShader_addSource(frag, VuoMesh_IndividualTriangles, NULL, NULL, maskFragmentShader);
	VuoRetain(frag);
	VuoShader_setUniform_VuoImage(frag, "texture", image);
	VuoShader_setUniform_VuoImage(frag, "mask", mask);
	*maskedImage = VuoImageRenderer_draw((*instance)->imageRenderer, frag, w, h, VuoImage_getColorDepth(image));

	VuoRelease(frag);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
