/**
 * @file
 * VuoImageBlur implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoImageBlur.h"
#include <OpenGL/CGLMacro.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageBlur",
					 "dependencies" : [
						 "VuoImageRenderer",
						 "VuoInteger",
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
	VuoShader shader;
	VuoShader discShader;
} VuoImageBlur_internal;

/**
 * Frees image blender state data.
 */
void VuoImageBlur_free(void *blur)
{
	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)blur;
	VuoRelease(bi->shader);
	VuoRelease(bi->discShader);
}

/**
 * Returns the value of the discretized, normalized gaussian function of `width` at `x`.
 *
 * `erf(x)` is the integral of the gaussian function from -inf to x.
 * So, find the integral of the gaussian function from x-delta to x+delta
 * by taking the difference between `erf()` at those positions.
 */
static double VuoImageBlur_gauss(double x, double width)
{
	return (erf((x+.5)*width) - erf((x-.5)*width))/2;
}

/**
 * Returns an image containing a set of (unnormalized) weights for the specified `radius`, depending on `shape`:
 *
 *    - discretized Gaussian weights
 *    - triangle weights (linear ramp from highest at center to lowest on edges)
 *    - box weights (all values equal except the last (left/right edge) which is fractional)
 *
 * Since the weights are symmetric, only the right half (including zero) is provided.
 */
static VuoImage VuoImageBlur_calculateWeights(VuoBlurShape shape, double radius)
{
	int pixelsWide;
	float *pixelFloats;
	if (shape == VuoBlurShape_Gaussian)
	{
		// Scale to the same apparent width as box.
		radius *= .6;

		const double width = 1. / (radius * sqrt(2));
		const double threshold = .1 / 255.;

		VuoList_VuoReal l = VuoListCreate_VuoReal();
		VuoLocal(l);
		pixelsWide = 0;
		double g = 1;
		while (g > threshold)
		{
			g = VuoImageBlur_gauss(pixelsWide, width);
			++pixelsWide;
			VuoListAppendValue_VuoReal(l, g);
		}

		// Don't try to create textures larger than OpenGL supports.
		pixelsWide = VuoInteger_clamp(pixelsWide, 0, 16384);

		VuoReal *pixels = VuoListGetData_VuoReal(l);
		pixelFloats = (float *)malloc(sizeof(float) * pixelsWide);
		for (int i = 0; i < pixelsWide; ++i)
			pixelFloats[i] = pixels[i];
	}
	else if (shape == VuoBlurShape_Linear)
	{
		// Scale to the same apparent width as box.
		radius *= 1.4;

		// When radius is 0, the half-triangle filter should be a single full-intensity pixel (identity).
		// When radius is 1, the half-triangle filter should be 2 pixels wide, the second pixel half the intensity of the first.

		pixelsWide = ceil(radius + 1);

		// Don't try to create textures larger than OpenGL supports.
		pixelsWide = VuoInteger_clamp(pixelsWide, 0, 16384);

		pixelFloats = (float *)malloc(sizeof(float) * pixelsWide);
		for (int i = 0; i < pixelsWide; ++i)
			pixelFloats[i] = 1 - (float)i/(radius + 1);
	}
	else // Box
	{
		// When radius is 0,   the half-box filter should be a single full-intensity pixel (identity).
		// When radius is 0.5, the half-box filter should be 2 pixels wide, the second pixel half the intensity of the first.
		// When radius is 1,   the half-box filter should be 2 pixels wide, same intensity for both pixels.

		pixelsWide = ceil(radius + 1);

		// Don't try to create textures larger than OpenGL supports.
		pixelsWide = VuoInteger_clamp(pixelsWide, 0, 16384);

		pixelFloats = (float *)malloc(sizeof(float) * pixelsWide);
		for (int i = 0; i < pixelsWide - 1; ++i)
			pixelFloats[i] = 1.;

		float lastPixel = radius - floor(radius);
		if (lastPixel == 0) lastPixel = 1;
		pixelFloats[pixelsWide - 1] = lastPixel;
	}

	return VuoImage_makeFromBuffer(pixelFloats, GL_LUMINANCE, pixelsWide, 1, VuoImageColorDepth_32, ^(void *buffer){ free(pixelFloats); });
}

/**
 * Creates state data for the image blurrer.
 */
VuoImageBlur VuoImageBlur_make(void)
{
	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)malloc(sizeof(VuoImageBlur_internal));
	VuoRegister(bi, VuoImageBlur_free);

	static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "VuoGlslRandom.glsl"
		\n#include "VuoGlslHsl.glsl"

		varying vec2 fragmentTextureCoordinate;

		uniform sampler2D texture;
		uniform sampler2D gaussianWeights;
		uniform float aspectRatio;

		uniform sampler2D mask;
		uniform bool hasMask;

		uniform float width; 	// render target width
		uniform float height; 	// render target height
		uniform float inset;
		uniform int gaussianWeightCount;
		uniform float quality;
		uniform bool radial;
		uniform int symmetry;
		uniform bool zoom;
		uniform vec2 center;
		uniform vec2 direction;

		float gaussianWeight(int x)
		{
			return texture2D(gaussianWeights, vec2((x + .5)/float(gaussianWeightCount), .5)).r;
		}

		vec4 sampleInset(vec2 uv)
		{
			if (inset > 0.)
				// Always return transparent black outside the texture (regardless of the texture's wrap mode).
				if (uv.x < 0. || uv.y < 0. || uv.x > 1. || uv.y > 1.)
					return vec4(0.);

			return VuoGlsl_sample(texture, uv);
		}

		void main(void)
		{
			vec2 uv = fragmentTextureCoordinate;

			// Inset the source image relative to the output image to account for the optional bleed.
			uv.x = (uv.x - inset/width)  *  width/(width -inset*2.);
			uv.y = (uv.y - inset/height) * height/(height-inset*2.);

			vec2 dir = direction;
			if (zoom)
				dir *= uv - center;

			float scale = 1.;
			if (hasMask)
			{
				vec4 maskColor = VuoGlsl_sample(mask, uv);

				// VuoGlsl_sample() returns premultiplied colors,
				// so we can take into account both the mask's luminance and alpha
				// by just looking at its (premultiplied) luminance.
				scale = VuoGlsl_rgbToHsl(maskColor.rgb).z;
				dir *= scale;
			}

			int iterations = int(ceil(float(gaussianWeightCount) * quality));

			vec2 fuzz = vec2(0.);
			if (quality < .75)
			{
				fuzz = VuoGlsl_random2D2D(uv);
				if (symmetry == 0)  // VuoCurveEasing_In
					fuzz *= .5;
				if (symmetry == 1)  // VuoCurveEasing_Out
					fuzz *= -.5;
				else if (symmetry == 2) // VuoCurveEasing_InOut
					fuzz -= vec2(.5, .5);

				fuzz *= dir * float(gaussianWeightCount) * 2. / float(1 + iterations);
			}

			float mixSum = gaussianWeight(0);
			vec4 colorSum = sampleInset(uv + fuzz) * mixSum;
			if (radial)
			{
				vec2 n = uv - center;
				n.y /= aspectRatio;
				float dist = distance(uv, center) * direction.x * scale;
				for (int i = 1; i < iterations; i++)
				{
					int is = int(float(i) / quality);
					float gw = gaussianWeight(is);
					if (symmetry == 0  // VuoCurveEasing_In
					 || symmetry == 2) // VuoCurveEasing_InOut
					{
						float c = cos(float(is)*dist);
						float s = sin(float(is)*dist);
						vec2 v = vec2(n.x*c - n.y*s,
									  n.x*s + n.y*c);
						v.y *= aspectRatio;
						colorSum += sampleInset(v + center + fuzz) * gw;
						mixSum += gw;
					}
					if (symmetry == 1  // VuoCurveEasing_Out
					 || symmetry == 2) // VuoCurveEasing_InOut
					{
						float c = cos(-float(is)*dist);
						float s = sin(-float(is)*dist);
						vec2 v = vec2(n.x*c - n.y*s,
									  n.x*s + n.y*c);
						v.y *= aspectRatio;
						colorSum += sampleInset(v + center + fuzz) * gw;
						mixSum += gw;
					}
				}
			}
			else // linear
			{
				for (int i = 1; i < iterations; i++)
				{
					int is = int(float(i) / quality);
					float gw = gaussianWeight(is);
					if (symmetry == 0  // VuoCurveEasing_In
					 || symmetry == 2) // VuoCurveEasing_InOut
					{
						colorSum += sampleInset(uv + dir * float(is) + fuzz) * gw;
						mixSum += gw;
					}
					if (symmetry == 1  // VuoCurveEasing_Out
					 || symmetry == 2) // VuoCurveEasing_InOut
					{
						colorSum += sampleInset(uv - dir * float(is) + fuzz) * gw;
						mixSum += gw;
					}
				}
			}

			gl_FragColor = colorSum / mixSum;
		}
	);

	bi->shader = VuoShader_make("Blur Shader");
	VuoShader_addSource(bi->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(bi->shader);

	static const char *discFragmentShader = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "VuoGlslRandom.glsl"
		\n#include "VuoGlslHsl.glsl"

		varying vec2 fragmentTextureCoordinate;

		uniform sampler2D texture;
		uniform sampler2D gaussianWeights;
		uniform float aspectRatio;

		uniform sampler2D mask;
		uniform bool hasMask;

		uniform float width;
		uniform float height;
		uniform float inset;
		uniform float radius;
		uniform float quality;

		vec4 sampleInset(vec2 uv)
		{
			if (inset > 0.)
				// Always return transparent black outside the texture (regardless of the texture's wrap mode).
				if (uv.x < 0. || uv.y < 0. || uv.x > 1. || uv.y > 1.)
					return vec4(0.);

			return VuoGlsl_sample(texture, uv);
		}

		void main(void)
		{
			vec2 uv = fragmentTextureCoordinate;
			float delta = fwidth(uv.x) * width/2.;

			// Inset the source image relative to the output image to account for the optional bleed.
			uv.x = (uv.x - inset/width)  *  width/(width -inset*2.);
			uv.y = (uv.y - inset/height) * height/(height-inset*2.);

			float maskedRadius = radius;
			if (hasMask)
			{
				vec4 maskColor = VuoGlsl_sample(mask, uv);

				// VuoGlsl_sample() returns premultiplied colors,
				// so we can take into account both the mask's luminance and alpha
				// by just looking at its (premultiplied) luminance.
				float maskAmount = VuoGlsl_rgbToHsl(maskColor.rgb).z;

				maskedRadius *= maskAmount;
			}

			vec2 fuzz = vec2(0.);
			if (quality < .75)
				fuzz = (VuoGlsl_random2D2D(uv) - vec2(.5,.5)) * (.75 - quality) * maskedRadius / vec2(width, height) / .75;

			int pixelRadius = int(ceil(maskedRadius*quality));
			vec4 colorSum = vec4(0.);
			float mixSum = 0.;
			for (int x = 0; x <= pixelRadius; ++x)
				for (int y = 0; y <= pixelRadius; ++y)
				{
					float dist = length(vec2(x,y) / quality);
					float f = smoothstep(maskedRadius + delta, maskedRadius - delta, dist);

					float sx = (float(x) / quality)/width;
					float sy = (float(y) / quality)/height;

					// Fourfold symmetry
					vec4 s = vec4(0.);
										  s += sampleInset(uv + vec2( sx, sy) + fuzz); mixSum += f;
					if (x > 0)          { s += sampleInset(uv + vec2(-sx, sy) + fuzz); mixSum += f; }
					if (y > 0)          { s += sampleInset(uv + vec2( sx,-sy) + fuzz); mixSum += f; }
					if (x > 0 && y > 0) { s += sampleInset(uv + vec2(-sx,-sy) + fuzz); mixSum += f; }
					colorSum += s * f;
				}

			gl_FragColor = colorSum / mixSum;
		}
	);

	bi->discShader = VuoShader_make("Disc Blur Shader");
	VuoShader_addSource(bi->discShader, VuoMesh_IndividualTriangles, NULL, NULL, discFragmentShader);
	VuoRetain(bi->discShader);

	return (VuoImageBlur)bi;
}

/**
 * Returns a blurred copy of @a image.  (Or, if radius is zero, returns @a image.)
 *
 * `radius` is in points, and respects the image's `scaleFactor`
 * (this function multiplies the two values).
 *
 * If @a expandBounds is true, the output image will be expanded to encompass the full bleed of the blur.
 * Otherwise, the output image will have the same dimensions as the soure image.
 */
VuoImage VuoImageBlur_blur(VuoImageBlur blur, VuoImage image, VuoImage mask, VuoBlurShape shape, VuoReal radius, VuoReal quality, VuoBoolean expandBounds)
{
	if (!image)
		return NULL;

	if (radius < 0.0001)
		return image;

	radius *= image->scaleFactor;

	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)blur;

	VuoImage gaussianWeights = VuoImageBlur_calculateWeights(shape, radius);
	VuoLocal(gaussianWeights);

	int inset = expandBounds ? gaussianWeights->pixelsWide-1 : 0;

	int w = image->pixelsWide + inset*2;
	int h = image->pixelsHigh + inset*2;

	if (shape == VuoBlurShape_Disc)
	{
		VuoShader_setUniform_VuoReal   (bi->discShader, "inset",               inset);
		VuoShader_setUniform_VuoReal   (bi->discShader, "width",               w);
		VuoShader_setUniform_VuoReal   (bi->discShader, "height",              h);
		VuoShader_setUniform_VuoImage  (bi->discShader, "texture",             image);
		VuoShader_setUniform_VuoImage  (bi->discShader, "mask",                mask);
		VuoShader_setUniform_VuoBoolean(bi->discShader, "hasMask",             mask ? true : false);
		VuoShader_setUniform_VuoReal   (bi->discShader, "radius",              radius);
		VuoShader_setUniform_VuoReal   (bi->discShader, "quality",             VuoReal_clamp(quality, 0.01, 1));

		VuoImage blurredImage = VuoImageRenderer_render(bi->discShader, w, h, VuoImage_getColorDepth(image));
		VuoImage_setWrapMode(blurredImage, VuoImage_getWrapMode(image));
		return blurredImage;
	}
	else // Gaussian, Triangle, Box
	{
		VuoShader_setUniform_VuoReal   (bi->shader, "inset",               inset);
		VuoShader_setUniform_VuoReal   (bi->shader, "width",               w);
		VuoShader_setUniform_VuoReal   (bi->shader, "height",              h);
		VuoShader_setUniform_VuoImage  (bi->shader, "texture",             image);
		VuoShader_setUniform_VuoImage  (bi->shader, "mask",                mask);
		VuoShader_setUniform_VuoBoolean(bi->shader, "hasMask",             mask ? true : false);
		VuoShader_setUniform_VuoImage  (bi->shader, "gaussianWeights",     gaussianWeights);
		VuoShader_setUniform_VuoInteger(bi->shader, "gaussianWeightCount", gaussianWeights->pixelsWide);
		VuoShader_setUniform_VuoReal   (bi->shader, "quality",             VuoReal_clamp(quality, 0, 1));
		VuoShader_setUniform_VuoBoolean(bi->shader, "radial",              false);
		VuoShader_setUniform_VuoInteger(bi->shader, "symmetry",            VuoCurveEasing_InOut);
		VuoShader_setUniform_VuoBoolean(bi->shader, "zoom",                false);

		VuoShader_setUniform_VuoPoint2d(bi->shader, "direction",           (VuoPoint2d){1./image->pixelsWide, 0});
		VuoImage horizontalPassImage = VuoImageRenderer_render(bi->shader, w, h, VuoImage_getColorDepth(image));
		VuoLocal(horizontalPassImage);

		VuoImageWrapMode wrapMode = VuoImage_getWrapMode(image);
		VuoImage_setWrapMode(horizontalPassImage, wrapMode);

		// Only inset the first pass.
		VuoShader_setUniform_VuoReal   (bi->shader, "inset",     0);

		VuoShader_setUniform_VuoImage  (bi->shader, "texture",   horizontalPassImage);
		VuoShader_setUniform_VuoPoint2d(bi->shader, "direction", (VuoPoint2d){0, 1.f/h});
		VuoImage bothPassesImage = VuoImageRenderer_render(bi->shader, w, h, VuoImage_getColorDepth(image));

		VuoImage_setWrapMode(bothPassesImage, wrapMode);

		return bothPassesImage;
	}
}

#ifndef DOXYGEN
/**
 * @deprecated Use VuoImageBlur_* instead.
 */
VuoImage VuoImage_blur(VuoImage image, VuoReal radius) __attribute__ ((deprecated("VuoImage_blur is deprecated, please use VuoImageBlur_* instead.")));
VuoImage VuoImage_blur(VuoImage image, VuoReal radius)
{
	VuoImageBlur ib = VuoImageBlur_make();
	VuoLocal(ib);
	return VuoImageBlur_blur(ib, image, NULL, VuoBlurShape_Gaussian, radius, 1, false);
}
#endif

/**
 * Returns a linearly-blurred copy of @a image.  (Or, if radius is zero, returns @a image.)
 *
 * `radius` is in points, and respects the image's `scaleFactor`
 * (this function multiplies the two values).
 */
VuoImage VuoImageBlur_blurDirectionally(VuoImageBlur blur, VuoImage image, VuoImage mask, VuoBlurShape shape, VuoReal radius, VuoReal quality, VuoReal angle, VuoBoolean symmetric, VuoBoolean expandBounds)
{
	if (!image)
		return NULL;

	if (radius < 0.0001)
		return image;

	radius *= image->scaleFactor;

	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)blur;

	VuoImage gaussianWeights = VuoImageBlur_calculateWeights(shape, radius);
	VuoLocal(gaussianWeights);

	int inset = expandBounds ? gaussianWeights->pixelsWide-1 : 0;

	int w = image->pixelsWide + inset*2;
	int h = image->pixelsHigh + inset*2;

	VuoShader_setUniform_VuoReal   (bi->shader, "inset",               inset);
	VuoShader_setUniform_VuoReal   (bi->shader, "width",               w);
	VuoShader_setUniform_VuoReal   (bi->shader, "height",              h);
	VuoShader_setUniform_VuoImage  (bi->shader, "texture",             image);
	VuoShader_setUniform_VuoImage  (bi->shader, "mask",                mask);
	VuoShader_setUniform_VuoBoolean(bi->shader, "hasMask",             mask ? true : false);
	VuoShader_setUniform_VuoImage  (bi->shader, "gaussianWeights",     gaussianWeights);
	VuoShader_setUniform_VuoInteger(bi->shader, "gaussianWeightCount", gaussianWeights->pixelsWide);
	VuoShader_setUniform_VuoReal   (bi->shader, "quality",             VuoReal_clamp(quality, 0, 1));
	VuoShader_setUniform_VuoBoolean(bi->shader, "radial",              false);
	VuoShader_setUniform_VuoInteger(bi->shader, "symmetry",            symmetric ? VuoCurveEasing_InOut : VuoCurveEasing_Out);
	VuoShader_setUniform_VuoBoolean(bi->shader, "zoom",                false);
	VuoShader_setUniform_VuoPoint2d(bi->shader, "direction",
		(VuoPoint2d){
			cos(angle*M_PI/180) / image->pixelsWide,
			sin(angle*M_PI/180) / image->pixelsHigh,
		});
	VuoImage blurredImage = VuoImageRenderer_render(bi->shader, w, h, VuoImage_getColorDepth(image));

	VuoImage_setWrapMode(blurredImage, VuoImage_getWrapMode(image));

	return blurredImage;
}

/**
 * Returns a radially-blurred copy of @a image.  (Or, if radius is zero, returns @a image.)
 *
 * `radius` is in points, and respects the image's `scaleFactor`
 * (this function multiplies the two values).
 */
VuoImage VuoImageBlur_blurRadially(VuoImageBlur blur, VuoImage image, VuoImage mask, VuoBlurShape shape, VuoPoint2d center, VuoReal radius, VuoReal quality, VuoDispersion dispersion, VuoCurveEasing symmetry, VuoBoolean expandBounds)
{
	if (!image)
		return NULL;

	if (radius < 0.0001)
		return image;

	radius *= image->scaleFactor;

	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)blur;

	// Make the apparent size of the radial blur roughly match that of the linear blur.
	if (dispersion == VuoDispersion_Radial)
		radius *= 2;

	VuoImage gaussianWeights = VuoImageBlur_calculateWeights(shape, radius);
	VuoLocal(gaussianWeights);

	int inset = expandBounds ? gaussianWeights->pixelsWide-1 : 0;

	int w = image->pixelsWide + inset*2;
	int h = image->pixelsHigh + inset*2;

	VuoShader_setUniform_VuoReal   (bi->shader, "inset",               inset);
	VuoShader_setUniform_VuoReal   (bi->shader, "width",               w);
	VuoShader_setUniform_VuoReal   (bi->shader, "height",              h);
	VuoShader_setUniform_VuoImage  (bi->shader, "texture",             image);
	VuoShader_setUniform_VuoImage  (bi->shader, "mask",                mask);
	VuoShader_setUniform_VuoBoolean(bi->shader, "hasMask",             mask ? true : false);
	VuoShader_setUniform_VuoImage  (bi->shader, "gaussianWeights",     gaussianWeights);
	VuoShader_setUniform_VuoInteger(bi->shader, "gaussianWeightCount", gaussianWeights->pixelsWide);
	VuoShader_setUniform_VuoReal   (bi->shader, "quality",             VuoReal_clamp(quality, 0, 1));
	VuoShader_setUniform_VuoBoolean(bi->shader, "radial",              dispersion == VuoDispersion_Radial ? true : false);
	VuoShader_setUniform_VuoInteger(bi->shader, "symmetry",            symmetry);
	VuoShader_setUniform_VuoBoolean(bi->shader, "zoom",                true);
	VuoShader_setUniform_VuoPoint2d(bi->shader, "center",              VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
	VuoShader_setUniform_VuoPoint2d(bi->shader, "direction",           (VuoPoint2d){1./image->pixelsWide, 1.f/image->pixelsWide});

	VuoImage blurredImage = VuoImageRenderer_render(bi->shader, w, h, VuoImage_getColorDepth(image));

	VuoImage_setWrapMode(blurredImage, VuoImage_getWrapMode(image));

	return blurredImage;
}
