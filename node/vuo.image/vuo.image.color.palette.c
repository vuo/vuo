/**
 * @file
 * vuo.image.color.palette node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <OpenGL/CGLMacro.h>

#include "VuoImageRenderer.h"

VuoModuleMetadata({
	"title": "Map Image to Palette",
	"keywords": [
		"filter",
		"replace", "apply", "recolor", "nearest", "convert", "match",
		"indexed color", "pseudocolor", "indirect color",
		"alias", "contour", "edge", "banding", "raster", "gif", "quantize", "gradient", "reduce", "depth",
		"retro", "1980s", "1990s",
		"white noise", "random", "scatter",
		"gradient noise", "perlin noise", "periodic noise",
		"ordered", "dot pattern", "Bayer 1973", "thatch", "crosshatch",
	],
	"version": "1.0.0",
	"dependencies": [
	],
	"node": {
		"exampleCompositions": [ "ComparePosterizeAndPalette.vuo", "MakeCyberspaceAvatar.vuo" ]
	}
});

static const char *fragmentShader = VUO_STRINGIFY(
	\n#include "VuoGlslAlpha.glsl"
	\n#include "VuoGlslRandom.glsl"
	\n#include "noise2D.glsl"

	varying vec2 fragmentTextureCoordinate;
	uniform sampler2D sourceImage;
	uniform sampler2D paletteImage;
	uniform sampler2D bayerMatrix;
	uniform int paletteCount;
	uniform int bayerMatrixDim;
	uniform float ditheringIntensity;
	uniform float aspectRatio;
	const float infinity = 1. / 0.;

	/**
	 * Returns the Bayer matrix value corresponding to this output pixel, normalized (0..1).
	 */
	float getBayerValue(void)
	{
		return texture2D(bayerMatrix, mod(gl_FragCoord.xy, vec2(bayerMatrixDim)) / bayerMatrixDim).r;
	}

	/**
	 * Returns the unpremultiplied palette color at the specified index.
	 */
	vec4 getPaletteColor(int i)
	{
		return VuoGlsl_sample(paletteImage, vec2((float(i) + .5) / float(paletteCount), .5));
	}

	void main()
	{
		vec4 sourceColor = VuoGlsl_sample(sourceImage, fragmentTextureCoordinate);

		// Unpremultiply, in order to compare full-brightness colors in the search below.
		sourceColor.rgb /= sourceColor.a > 0. ? sourceColor.a : 1.;

		\n#if DITHERING_MODE == 0\n  // White noise

			sourceColor.rgb += (VuoGlsl_random2D3D(fragmentTextureCoordinate) - .5) * ditheringIntensity;

		\n#elif DITHERING_MODE == 10\n  // Gradient noise

			sourceColor.rgb += (snoise2D3D(fragmentTextureCoordinate * vec2(1., 1./aspectRatio) * 2 / 0.05)) * ditheringIntensity;

		\n#elif DITHERING_MODE == 11\n  // Gradient noise

			sourceColor.rgb += (snoise2D3D(fragmentTextureCoordinate * vec2(1., 1./aspectRatio) *2 / 0.02)) * ditheringIntensity;

		\n#elif DITHERING_MODE == 12\n  // Gradient noise

			sourceColor.rgb += (snoise2D3D(fragmentTextureCoordinate * vec2(1., 1./aspectRatio) *2 / 0.01)) * ditheringIntensity;

		\n#else\n  // Bayer sequence

			// Compensate for Bayer brightening.
			// (Empirically determined by quantizing some nature photos down to 8 or 16 colors,
			// and trying to gauge their overall lightness by squinting while sweeping the ditheringIntensity slider.)
			sourceColor.rgb = pow(sourceColor.rgb, vec3(1. + ditheringIntensity / 2.));

			// The Bayer sequence is normalized 0..1;
			// offset it so that it (roughly) equally darkens and lightens the image.
			float v = getBayerValue();
			sourceColor.rgb += (v - .5) * ditheringIntensity;

		\n#endif\n

		vec4 nearestPaletteColor = vec4(-1);
		float shortestDistance = infinity;
		for (int c = 0; c < paletteCount; ++c)
		{
			vec4 paletteColor = getPaletteColor(c);
			float rd = paletteColor.r - sourceColor.r;
			float gd = paletteColor.g - sourceColor.g;
			float bd = paletteColor.b - sourceColor.b;
			float distance = rd * rd + gd * gd + bd * bd;
			if (distance < shortestDistance)
			{
				nearestPaletteColor = paletteColor;
				shortestDistance = distance;
			}
		}

		// The framebuffer expects a premultiplied color; combine alpha from the source image and palette.
		float outputAlpha = nearestPaletteColor.a * sourceColor.a;
		gl_FragColor = vec4(nearestPaletteColor.rgb * outputAlpha, outputAlpha);
	}
);


/**
 * Based on <https://github.com/tromero/BayerMatrix>.
 */
static void vuo_image_color_palette_makeBayer_recurse(__fp16 *buf, unsigned int dimOriginal, unsigned int dimCurrent, unsigned int x, unsigned int y, unsigned int step, unsigned int value)
{
	if (dimCurrent == 1)
	{
		buf[y * dimOriginal + x] = value / (float)(dimOriginal * dimOriginal - 1);
		return;
	}

	unsigned int dimHalf = dimCurrent / 2;
	vuo_image_color_palette_makeBayer_recurse(buf, dimOriginal, dimHalf, x,           y,           step * 4, value + step * 0);
	vuo_image_color_palette_makeBayer_recurse(buf, dimOriginal, dimHalf, x + dimHalf, y + dimHalf, step * 4, value + step * 1);
	vuo_image_color_palette_makeBayer_recurse(buf, dimOriginal, dimHalf, x + dimHalf, y,           step * 4, value + step * 2);
	vuo_image_color_palette_makeBayer_recurse(buf, dimOriginal, dimHalf, x,           y + dimHalf, step * 4, value + step * 3);
}

/**
 * Returns an image containing the Bayer matrix of the specified exponent.
 * (e.g., exponent 3 produces an 8x8 matrix)
 */
static VuoImage vuo_image_color_palette_makeBayer(unsigned char exponent)
{
	unsigned int dim = exp2(exponent);
	__fp16 *buf = malloc(sizeof(__fp16) * dim * dim);
	vuo_image_color_palette_makeBayer_recurse(buf, dim, dim, 0, 0, 1, 0);

	if (VuoIsDebugEnabled())
	{
		fprintf(stderr, "Bayer exponent %3u (%ux%u)\n", exponent, dim, dim);
		for (int y = 0; y < dim; ++y)
		{
			for (int x = 0; x < dim; ++x)
				fprintf(stderr, "%5.3f ", buf[y * dim + x]);
			fprintf(stderr, "\n");
		}
	}

	return VuoImage_makeFromBuffer(buf, GL_LUMINANCE, dim, dim, VuoImageColorDepth_16, ^(void *buffer){ free(buffer); });
}

struct nodeInstanceData
{
	VuoShader shader;

	struct
	{
		VuoInteger ditheringMode;
	} priorSettings;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = NULL;

	instance->priorSettings.ditheringMode = -1;

	return instance;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoList_VuoColor, {"default":["#000", "#fff"]}) palette,
	VuoInputData(VuoInteger, {"menuItems":[
		"Noise",
		{"value":  0, "name":"    White Noise"},
		{"value": 10, "name":"    Gradient Noise 0.05"},
		{"value": 11, "name":"    Gradient Noise 0.02"},
		{"value": 12, "name":"    Gradient Noise 0.01"},
		"Ordered",
		{"value": 20, "name":"    Bayer 2x2"},
		{"value": 24, "name":"    Bayer 3x3"},
		{"value": 21, "name":"    Bayer 4x4"},
		{"value": 22, "name":"    Bayer 8x8"},
		{"value": 23, "name":"    Bayer 16x16"},
//		"Error Diffusion",
//		{"value": 40, "name":"    Floyd-Steinberg"}, // @@@
	], "default":20}) dithering,
	VuoInputData(VuoReal, {"default":0.25, "suggestedMin":0, "suggestedMax":1}) ditheringIntensity,
	VuoOutputData(VuoImage) quantizedImage)
{
	if (!image || VuoListGetCount_VuoColor(palette) == 0)
	{
		*quantizedImage = NULL;
		return;
	}

	VuoImage bayerMatrix = NULL;
	if ((*instance)->priorSettings.ditheringMode != dithering)
	{
		VuoRelease((*instance)->shader);

		char *sourceWithPrefix = VuoText_format("#version 120\n#define DITHERING_MODE %d\n\n%s", (int)dithering, fragmentShader);

		(*instance)->shader = VuoShader_make("Map Image to Palette Shader");
		VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, sourceWithPrefix);
		free(sourceWithPrefix);
		VuoRetain((*instance)->shader);

		if (dithering >= 20 && dithering <= 24)
		{
			if (dithering == 24)
			{
				// This 3x3 matrix can't be generated by the above algorithm (its dimension isn't a power of 2),
				// and the ordering isn't great (it produces diagonal tiling artifacts),
				// but it still could be useful since it generates unique results.
				unsigned char buf[] = {
					  0, 192,  96,
					224, 160,  64,
					128,  32, 255,
				};
				bayerMatrix = VuoImage_makeFromBuffer(buf, GL_LUMINANCE, 3, 3, VuoImageColorDepth_8, ^(void *buffer){});
				VuoShader_setUniform_VuoInteger((*instance)->shader, "bayerMatrixDim", 3);
			}
			else
			{
				int bayerExponent = dithering - 19;
				bayerMatrix = vuo_image_color_palette_makeBayer(bayerExponent);
				VuoShader_setUniform_VuoInteger((*instance)->shader, "bayerMatrixDim", exp2(bayerExponent));
			}
		}

		(*instance)->priorSettings.ditheringMode = dithering;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "sourceImage", image);
	if (bayerMatrix)
		VuoShader_setUniform_VuoImage((*instance)->shader, "bayerMatrix", bayerMatrix);

	VuoRetain(palette);
	VuoColor *paletteData = VuoListGetData_VuoColor(palette);
	unsigned int paletteCount = VuoListGetCount_VuoColor(palette);

	// In VuoColor, RGB are not premultiplied by A.
	// Pass these original (unpremultiplied) RGB values to the shader (for use when color-matching),
	// and pass the alpha (for use when substituting colors into the output image).
	VuoImage paletteImage = VuoImage_makeFromBuffer(paletteData, GL_RGBA, paletteCount, 1, VuoImageColorDepth_32, ^(void *buffer){ VuoRelease(palette); });
	VuoShader_setUniform_VuoImage  ((*instance)->shader, "paletteImage", paletteImage);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "paletteCount", paletteCount);

	VuoShader_setUniform_VuoReal((*instance)->shader, "ditheringIntensity", ditheringIntensity);

	*quantizedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
