/**
 * @file
 * VuoImageBlend implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageBlend.h"
#include "VuoImageRenderer.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageBlend",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ]
				 });
#endif
/// @}

/**
 * State data for the image blender.
 */
typedef struct
{
	VuoShader shader;
	VuoGlContext context;
	VuoImageRenderer imageRenderer;
} VuoImageBlend_internal;

/**
 * Frees image blender state data.
 */
void VuoImageBlend_free(void *blend)
{
	VuoImageBlend_internal *bi = (VuoImageBlend_internal *)blend;
	VuoRelease(bi->shader);
	VuoRelease(bi->imageRenderer);
	VuoGlContext_disuse(bi->context);
}

/**
 * Creates state data for the image blender.
 */
VuoImageBlend VuoImageBlend_make(void)
{
	const char *shaderSource = VUOSHADER_GLSL_SOURCE(120,
		varying vec4 fragmentTextureCoordinate;
		uniform sampler2D textureA;
		uniform sampler2D textureB;
		uniform float factor;

		void main(void)
		{
			gl_FragColor = mix(texture2D(textureA, fragmentTextureCoordinate.xy), texture2D(textureB, fragmentTextureCoordinate.xy), factor);
		}
	);

	VuoImageBlend_internal *bi = (VuoImageBlend_internal *)malloc(sizeof(VuoImageBlend_internal));
	VuoRegister(bi, VuoImageBlend_free);

	bi->shader = VuoShader_make("Blend Image Shader");
	VuoRetain(bi->shader);
	VuoShader_addSource(bi->shader, VuoMesh_IndividualTriangles, NULL, NULL, shaderSource);

	bi->context = VuoGlContext_use();

	bi->imageRenderer = VuoImageRenderer_make(bi->context);
	VuoRetain(bi->imageRenderer);

	return (VuoImageBlend)bi;
}

/**
 * Averages multiple images together into a single image.
 *
 * The output image is the same size as the first image (later images are stretched if needed).
 *
 * The returned image has retain count +1.
 */
VuoImage VuoImageBlend_blend(VuoImageBlend blend, VuoList_VuoImage images)
{
	unsigned long imageCount = VuoListGetCount_VuoImage(images);
	if (imageCount == 0)
		return NULL;

	VuoImage blendedImage = VuoListGetValue_VuoImage(images, 1);
	if (imageCount == 1)
	{
		VuoRetain(blendedImage);
		return blendedImage;
	}

	VuoImageColorDepth cd = VuoImage_getColorDepth(blendedImage);

	/// @todo Could maybe improve performance by blending more than one image per iteration.

	VuoImageBlend_internal *bi = (VuoImageBlend_internal *)blend;
	VuoRetain(blendedImage);
	double factor = 1.;
	for (unsigned long i = 2; i <= imageCount; ++i)
	{
		VuoImage image = VuoListGetValue_VuoImage(images, i);

		VuoShader_setUniform_VuoImage(bi->shader, "textureA",	blendedImage);
		VuoShader_setUniform_VuoImage(bi->shader, "textureB",	image);

		factor *= ((double)i - 1.) / i;
		VuoShader_setUniform_VuoReal (bi->shader, "factor",		factor);

		VuoImage b = VuoImageRenderer_draw(bi->imageRenderer, bi->shader, blendedImage->pixelsWide, blendedImage->pixelsHigh, cd);
		VuoRetain(b);
		VuoRelease(blendedImage);
		blendedImage = b;
	}

	return blendedImage;
}
