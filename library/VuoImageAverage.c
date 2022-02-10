/**
 * @file
 * VuoImageAverage implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageAverage.h"
#include "VuoImageRenderer.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageAverage",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ]
				 });
#endif
/// @}

/**
 * State data for the image averager.
 */
typedef struct
{
	VuoShader shader;
} VuoImageAverage_internal;

/**
 * Frees image averager state data.
 */
void VuoImageAverage_free(void *average)
{
	VuoImageAverage_internal *bi = (VuoImageAverage_internal *)average;
	VuoRelease(bi->shader);
	free(bi);
}

/**
 * Creates state data for the image averager.
 */
VuoImageAverage VuoImageAverage_make(void)
{
	const char *shaderSource = VUOSHADER_GLSL_SOURCE(120,
		varying vec2 fragmentTextureCoordinate;
		uniform sampler2D textureA;
		uniform sampler2D textureB;
		uniform float factor;

		void main(void)
		{
			vec4 colorA = texture2D(textureA, fragmentTextureCoordinate);
			vec4 colorB = texture2D(textureB, fragmentTextureCoordinate);
			vec4 mixed = mix(colorA, colorB, factor);
			gl_FragColor = mixed;
		}
	);

	VuoImageAverage_internal *bi = (VuoImageAverage_internal *)malloc(sizeof(VuoImageAverage_internal));
	VuoRegister(bi, VuoImageAverage_free);

	bi->shader = VuoShader_make("Average Image Shader");
	VuoRetain(bi->shader);
	VuoShader_addSource(bi->shader, VuoMesh_IndividualTriangles, NULL, NULL, shaderSource);

	return (VuoImageAverage)bi;
}

/**
 * Averages multiple images together into a single image.
 *
 * The output image is the same size as the first image (later images are stretched if needed).
 *
 * The returned image has retain count +1.
 */
VuoImage VuoImageAverage_average(VuoImageAverage average, VuoList_VuoImage images)
{
	unsigned long imageCount = VuoListGetCount_VuoImage(images);
	if (imageCount == 0)
		return NULL;

	VuoImage averagedImage = VuoListGetValue_VuoImage(images, 1);
	if (imageCount == 1)
	{
		VuoRetain(averagedImage);
		return averagedImage;
	}

	VuoImageColorDepth cd = VuoImage_getColorDepth(averagedImage);

	/// @todo Could maybe improve performance by blending more than one image per iteration.

	VuoImageAverage_internal *bi = (VuoImageAverage_internal *)average;
	VuoRetain(averagedImage);
	double factor = 1.;
	for (unsigned long i = 2; i <= imageCount; ++i)
	{
		VuoImage image = VuoListGetValue_VuoImage(images, i);

		VuoShader_setUniform_VuoImage(bi->shader, "textureA", averagedImage);
		VuoShader_setUniform_VuoImage(bi->shader, "textureB", image);

		factor *= ((double)i - 1.) / i;
		VuoShader_setUniform_VuoReal(bi->shader, "factor", factor);

		VuoImage b = VuoImageRenderer_render(bi->shader, averagedImage->pixelsWide, averagedImage->pixelsHigh, cd);
		VuoRetain(b);
		VuoRelease(averagedImage);
		averagedImage = b;
	}

	return averagedImage;
}
