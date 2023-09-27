/**
 * @file
 * VuoImageDilate implementation.
 *
 * @copyright Copyright © 2012–2019 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"
#include "VuoImageDilate.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoImageDilate",
	"dependencies" : [
		"VuoImageRenderer",
		"VuoReal",
	]
});
#endif
/// @}

/**
 * State data for the image blurrer.
 */
typedef struct
{
	VuoShader shader[2][4];  // [func][shape]
} VuoImageDilate_internal;

/**
 * Frees image blender state data.
 */
void VuoImageDilate_free(void *blur)
{
	VuoImageDilate_internal *bi = (VuoImageDilate_internal *)blur;
	for (int func = 0; func <= 1; ++func)
		for (int shape = VuoBlurShape_Box; shape <= VuoBlurShape_Disc; ++shape)
			VuoRelease(bi->shader[func][shape]);
	free(bi);
}

/**
 * Creates state data for the image blurrer.
 */
VuoImageDilate VuoImageDilate_make(void)
{
	VuoImageDilate_internal *bi = (VuoImageDilate_internal *)malloc(sizeof(VuoImageDilate_internal));
	VuoRegister(bi, VuoImageDilate_free);

	static const char *fragmentShader = VUO_STRINGIFY(
		\n#include "VuoGlslAlpha.glsl"
		\n#include "VuoGlslBrightness.glsl"

		// Inputs
		uniform sampler2D texture;
		uniform vec2 viewportSize;
		uniform float halfWidth;
		varying vec2 fragmentTextureCoordinate;

		\n#if FUNC == 0\n
			\n#define func max\n
			\n#define a 0\n
		\n#else\n
			\n#define func min\n
			\n#define a 1\n
		\n#endif\n

		float straightstep(float edge0, float edge1, float x)
		{
			return clamp((x - edge0) / (edge1 - edge0), 0., 1.);
		}

		void main()
		{
			vec4 c = vec4(a);

			float delta = fwidth(fragmentTextureCoordinate.x) * viewportSize.x/2.;

			\n#if SHAPE == 2\n // Box

				int steps = int(halfWidth) + 1;

				for (int y = -steps; y <= steps; ++y)
					for (int x = -steps; x <= steps; ++x)
					{
						float f = straightstep(halfWidth + delta, halfWidth - delta, abs(float(x)))
							* straightstep(halfWidth + delta, halfWidth - delta, abs(float(y)));
						c = mix(c, func(c, VuoGlsl_sample(texture, fragmentTextureCoordinate + vec2(float(x)/viewportSize.x, float(y)/viewportSize.y))), f);
					}

			\n#else\n // Disc

				for (int y = 0; y <= halfWidth + 1; ++y)
					for (int x = 0; x <= halfWidth + 1; ++x)
					{
						float dist = length(vec2(x,y));
						float f = straightstep(halfWidth + delta, halfWidth - delta, dist);
						float sx = float(x)/viewportSize.x;
						float sy = float(y)/viewportSize.y;

						// Fourfold symmetry
						c = mix(c, func(c, VuoGlsl_sample(texture, fragmentTextureCoordinate + vec2( sx,  sy))), f);
						c = mix(c, func(c, VuoGlsl_sample(texture, fragmentTextureCoordinate + vec2(-sx,  sy))), f);
						c = mix(c, func(c, VuoGlsl_sample(texture, fragmentTextureCoordinate + vec2( sx, -sy))), f);
						c = mix(c, func(c, VuoGlsl_sample(texture, fragmentTextureCoordinate + vec2(-sx, -sy))), f);
					}

			\n#endif\n

			gl_FragColor = c;
		}
	);

	for (int func = 0; func <= 1; ++func)
		for (int shape = VuoBlurShape_Box; shape <= VuoBlurShape_Disc; ++shape)
		{
			char *sourceWithPrefix = VuoText_format("#version 120\n#define FUNC %d\n#define SHAPE %d\n\n%s", func, shape, fragmentShader);
			bi->shader[func][shape] = VuoShader_make("Morphology Shader");
			VuoShader_addSource(bi->shader[func][shape], VuoMesh_IndividualTriangles, NULL, NULL, sourceWithPrefix);
			VuoRetain(bi->shader[func][shape]);
		}

	return (VuoImageDilate)bi;
}

/**
 * Returns a dilated copy of @a image.  (Or, if radius is zero, returns @a image.)
 */
VuoImage VuoImageDilate_dilate(VuoImageDilate dilate, VuoImage image, VuoBlurShape shape, VuoReal radius, VuoBoolean rescind)
{
	if (!image)
		return NULL;

	radius *= image->scaleFactor;

	if (VuoReal_areEqual(radius, 0))
		return image;

	VuoImageDilate_internal *bi = (VuoImageDilate_internal *)dilate;

	int func = radius >= 0 ? 0 : 1;
	int shape2 = MAX(MIN(shape, VuoBlurShape_Disc), VuoBlurShape_Box);

	VuoShader_setUniform_VuoImage(bi->shader[func][shape2], "texture", image);
	VuoShader_setUniform_VuoReal(bi->shader[func][shape2], "halfWidth", fabs(radius) + .5);

	VuoImage dilatedImage = VuoImageRenderer_render(bi->shader[func][shape2], image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	if (rescind)
	{
		VuoImage d = dilatedImage;
		VuoLocal(d);

		int otherFunc = 1-func;
		VuoShader_setUniform_VuoImage(bi->shader[otherFunc][shape2], "texture", d);
		VuoShader_setUniform_VuoReal(bi->shader[otherFunc][shape2], "halfWidth", fabs(radius) + .5);

		dilatedImage = VuoImageRenderer_render(bi->shader[otherFunc][shape2], image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
	}

	return dilatedImage;
}
