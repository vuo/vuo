/**
 * @file
 * VuoImageConvolve implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoImageConvolve.h"
#include <OpenGL/CGLMacro.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageConvolve",
					 "dependencies" : [
						 "VuoImageRenderer",
						 "VuoInteger",
						 "VuoReal",
					 ]
				 });
#endif
/// @}

/**
 * State data for the image convolver.
 */
typedef struct
{
	VuoShader shader;
} VuoImageConvolve_internal;

/**
 * Frees image blender state data.
 */
void VuoImageConvolve_free(void *convolve)
{
	VuoImageConvolve_internal *bi = (VuoImageConvolve_internal *)convolve;
	VuoRelease(bi->shader);
}

/**
 * Returns the (0,0)-centered Laplacian-of-Gaussian function at the given position and sigma.
 * (This is the continuous version of the function, but we make up for it by normalizing
 * and removing the DC offset in @ref VuoImageConvolve_generateMatrix.)
 */
double VuoImageConvolve_laplacianOfGaussian(double x, double y, double radius)
{
	const double sigma = radius * .6 * sqrt(2);
	double r2 = x*x + y*y;
	// Clamp normalization term to minimum of 1, to keep the DC offset low for small sigma values.
	return -10. / sqrt(2. * M_PI * fmax(1, sigma * sigma * sigma * sigma))
		 * (1. - r2/(2*sigma*sigma))
		 * exp(-r2/(2*sigma*sigma));
}

/**
 * Returns a recommended matrix width for @ref VuoImageConvolve_laplacianOfGaussian.
 */
unsigned int VuoImageConvolve_laplacianOfGaussianWidth(double radius)
{
	// Kernel width estimation from
	// "On the discrete representation of the Laplacian of Gaussian" by Steve R. Gunn
	// (Pattern Recognition 32 (1999), page 1463 - 1472).
	// µₛ where epsilon < 10^-6.
	const double mu_s = 5;

	// Scale as in VuoImageBlur_calculateWeights().
	const double sigma = radius * .6 * sqrt(2);

	return 1 + 2 * ceil(mu_s * sigma);
}

/**
 * Returns an image containing a set of (unnormalized) weights for the specified function.
 */
VuoImage VuoImageConvolve_generateMatrix(VuoImageConvolveFunction f, unsigned int width, bool removeDCOffset, double param)
{
	// Width must be odd.
	int pixelsWide = width | 1;
//	printf("pixelsWide = %d\n", pixelsWide);

	float *pixelFloats = (float *)malloc(sizeof(float) * pixelsWide * pixelsWide);
	double sum = 0;
	for (int y = 0; y < pixelsWide; ++y)
		for (int x = 0; x < pixelsWide; ++x)
		{
			float v = f(x - pixelsWide/2, y - pixelsWide/2, param);
			pixelFloats[y * pixelsWide + x] = v;
			sum += v;
		}

	if (removeDCOffset)
	{
		double offset = sum / (pixelsWide * pixelsWide);
		for (int y = 0; y < pixelsWide; ++y)
			for (int x = 0; x < pixelsWide; ++x)
				pixelFloats[y * pixelsWide + x] -= offset;
	}

//	double sum2 = 0;
//	for (int y = 0; y < pixelsWide; ++y)
//	{
//		for (int x = 0; x < pixelsWide; ++x)
//		{
//			sum2 += pixelFloats[y * pixelsWide + x];
//			printf("%9.5f ", pixelFloats[y * pixelsWide + x]);
//		}
//		printf("\n");
//	}
//	if (removeDCOffset)
//		printf("sum before removing DC offset = %g    after = %g\n", sum, sum2);
//	else
//		printf("sum = %g\n", sum);

	return VuoImage_makeFromBuffer(pixelFloats, GL_LUMINANCE, pixelsWide, pixelsWide, VuoImageColorDepth_16, ^(void *buffer){ free(pixelFloats); });
}

/**
 * Creates state data for the image Convolverer.
 */
VuoImageConvolve VuoImageConvolve_make(void)
{
	VuoImageConvolve_internal *bi = (VuoImageConvolve_internal *)malloc(sizeof(VuoImageConvolve_internal));
	VuoRegister(bi, VuoImageConvolve_free);

	static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
		include(VuoGlslAlpha)
		include(VuoGlslBrightness)

		varying vec4 fragmentTextureCoordinate;

		uniform sampler2D image;
		uniform vec2 viewportSize;
		uniform sampler2D convolutionMatrix;
		uniform int convolutionMatrixWidth;
		uniform int channels;
		uniform float intensity;
		uniform int range;

		float weight(int x, int y)
		{
			return texture2D(convolutionMatrix, vec2(x + .5, y + .5) / float(convolutionMatrixWidth)).r;
		}

		void main(void)
		{
			vec2 uv = fragmentTextureCoordinate.xy;

			vec4 colorSum = vec4(0.);

			for (int y = 0; y < convolutionMatrixWidth; y++)
				for (int x = 0; x < convolutionMatrixWidth; x++)
				{
					float w = weight(x,y);
					vec4 color = VuoGlsl_sample(image, uv + vec2(
						(x - convolutionMatrixWidth/2)/viewportSize.x,
						(y - convolutionMatrixWidth/2)/viewportSize.y
						));
					color = VuoGlsl_gray(color, channels) * w;
					colorSum += color;
				}

			colorSum.rgb *= intensity;

			vec4 color = VuoGlsl_sample(image, uv);

			if (range == 0)      // VuoDiode_Unipolar
				colorSum.rgb = clamp(colorSum.rgb / 2. + .5, 0, color.a);
			else if (range == 1) // VuoDiode_Bipolar
				colorSum.rgb = clamp(colorSum.rgb, 0, color.a);
			else if (range == 2) // VuoDiode_Absolute
				colorSum.rgb = clamp(abs(colorSum.rgb), 0, color.a);

			gl_FragColor = vec4(colorSum.rgb, color.a);
		}
	);

	bi->shader = VuoShader_make("General Convolution Shader");
	VuoShader_addSource(bi->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(bi->shader);

	return (VuoImageConvolve)bi;
}

/**
 * Returns a convolved copy of @a image.  (Or, if radius is zero, returns @a image.)
 *
 * `convolutionMatrix` must be square and have odd width/height.
 */
VuoImage VuoImageConvolve_convolve(VuoImageConvolve convolve, VuoImage image, VuoImage convolutionMatrix, VuoThresholdType channels, double intensity, VuoDiode range)
{
	if (!VuoImage_isPopulated(image))
		return NULL;

	if (!VuoImage_isPopulated(convolutionMatrix)
	 || convolutionMatrix->pixelsWide != convolutionMatrix->pixelsHigh
	 || !(convolutionMatrix->pixelsWide & 1)
	 || !(convolutionMatrix->pixelsHigh & 1))
		return NULL;

	VuoImageConvolve_internal *bi = (VuoImageConvolve_internal *)convolve;

	VuoShader_setUniform_VuoImage  (bi->shader, "image",                  image);
	VuoShader_setUniform_VuoImage  (bi->shader, "convolutionMatrix",      convolutionMatrix);
	VuoShader_setUniform_VuoInteger(bi->shader, "convolutionMatrixWidth", convolutionMatrix->pixelsWide);
	VuoShader_setUniform_VuoInteger(bi->shader, "channels",               channels);
	VuoShader_setUniform_VuoReal   (bi->shader, "intensity",              intensity);
	VuoShader_setUniform_VuoInteger(bi->shader, "range",                  range);

	VuoImage convolvedImage = VuoImageRenderer_render(bi->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
	VuoImage_setWrapMode(convolvedImage, VuoImage_getWrapMode(image));
	return convolvedImage;
}
