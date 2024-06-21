/**
 * @file
 * vuo.image.color.palette node implementation.
 *
 * @copyright Copyright © 2012–2024 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <OpenGL/CGLMacro.h>
#include <simd/vector_types.h>
#include <simd/math.h>

#include "VuoErrorDiffusionDither.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
	"title": "Map Image to Palette",
	"keywords": [
		"filter",
		"replace", "apply", "recolor", "nearest", "convert", "match",
		"indexed color", "pseudocolor", "indirect color",
		"alias", "contour", "edge", "banding", "raster", "gif", "quantize", "gradient", "reduce", "depth",
		"retro", "1970s", "1980s", "1990s",
		"black and white", "bi-level", "bilevel",
		"halftoning", "printing", "printer", "dot matrix",
		"grayscale", "greyscale",
		"white noise", "random", "scatter",
		"gradient noise", "perlin noise", "periodic noise",
		"ordered", "dot pattern", "Bryce Bayer", "thatch", "crosshatch", "Kodak",
		"error diffusion", "correcting", "correction", "dispersion", "distribution", "adaptive", "spatial",
			"south", "east", "southeast",
			"Robert W. Floyd", "Louis Steinberg", "false",
			"John F. Jarvis", "C. N. Judice", "William H. Ninke", "JaJuNi", "Bell Labs",
			"Peter Stucki", "MECCA", "IBM",
			"Daniel Burkes", "TerraVision",
			"Bill Atkinson", "MacPaint", "HyperScan", "HyperCard",
			"Frankie Sierra", "three-line", "two-line", "filter lite",
			"Jeng-nan Shiau", "Zhigang Fan",
			"Steven Pigeon",
			"Lee Daniel Crocker", "Paul Boulay", "Mike Morra", "Epson",
	],
	"version": "1.1.0",
	"dependencies": [
		"VuoErrorDiffusionDither",
	],
	"node": {
		"exampleCompositions": [ "CompareDitheringAlgorithms.vuo", "CompareDitheringIntensities.vuo", "CompareDitheringPaletteColors.vuo",
								 "ComparePaletteMatchingAccuracy.vuo", "ComparePosterizeAndPalette.vuo", "MakeCyberspaceAvatar.vuo" ]
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
	uniform float colorDistanceThreshold;
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
	 * Returns the unpremultiplied, sRGB palette color at the specified index.
	 */
	vec4 getPaletteColor(int i)
	{
		return VuoGlsl_sample(paletteImage, vec2((float(i) + .5) / float(paletteCount), .5));
	}

	/**
	 * Converts sRGB-D65 -> Linear RGB-D65.
	 */
	vec3 sRGBToLinearRGB(vec3 sRGB)
	{
		return vec3(
			sRGB.r <= 0.04045 ? sRGB.r/12.92 : pow((sRGB.r+0.055)/1.055,2.4),
			sRGB.g <= 0.04045 ? sRGB.g/12.92 : pow((sRGB.g+0.055)/1.055,2.4),
			sRGB.b <= 0.04045 ? sRGB.b/12.92 : pow((sRGB.b+0.055)/1.055,2.4));
	}

	/**
	 * Converts Linear RGB-D65 -> YUV/YCbCr (broadcast-limited-luma, SD / BT.601).
	 */
	vec3 linearRGBToYUV(vec3 linearRGB)
	{
		// https://support.medialooks.com/hc/en-us/articles/360030737152-Color-correction-with-matrix-transformation
		mat4 linearRGBToYCbCr = mat4(
			 .257,  .504,  .098,  .0625,
			-.148, -.291,  .439,  .5   ,
			 .439, -.368, -.071,  .5   ,
			 .0  ,  .0  ,  .0  , 1.    );
		return (vec4(linearRGB, 1.) * linearRGBToYCbCr).rgb;
	}

	void main()
	{
		vec4 sourceColor = VuoGlsl_sample(sourceImage, fragmentTextureCoordinate);

		// Unpremultiply, in order to compare full-brightness colors in the search below.
		sourceColor.rgb /= sourceColor.a > 0. ? sourceColor.a : 1.;

		vec3 ditherOffset = vec3(0);

		\n#if DITHERING_MODE == 0\n  // White noise

			ditherOffset = (VuoGlsl_random2D3D(fragmentTextureCoordinate) - .5) * 1.3;

		\n#elif DITHERING_MODE == 10\n  // Gradient noise

			ditherOffset = snoise2D3D(fragmentTextureCoordinate * vec2(1., 1./aspectRatio) * 2 / 0.05) * 1.3;

		\n#elif DITHERING_MODE == 11\n  // Gradient noise

			ditherOffset = snoise2D3D(fragmentTextureCoordinate * vec2(1., 1./aspectRatio) * 2 / 0.02) * 1.3;

		\n#elif DITHERING_MODE == 12\n  // Gradient noise

			ditherOffset = snoise2D3D(fragmentTextureCoordinate * vec2(1., 1./aspectRatio) * 2 / 0.01) * 1.3;

		\n#else\n  // Bayer sequence

			ditherOffset = vec3(getBayerValue());

			// The Bayer sequence is normalized 0..1;
			// offset it so that it (roughly) equally darkens and lightens the image.
			ditherOffset -= 0.5;

		\n#endif\n


		// Emperically-determined values resulting in:
		// - a small (5%) light plateau across all bayerMatrixDims and all paletteCounts (good)
		// - a large (10%-30%) dark plateau across all bayerMatrixDims and all paletteCounts (a 30% plateau for paletteCount=2 seems excessive but I don't know how to reduce that without reducing quality of other combinations)
		// - when squinting, equal brightness across all bayerMatrixDims and all paletteCounts (good)
		ditherOffset += 0.15;
		ditherOffset *= 0.75;

		// Apply ditherOffset to the sRGB value (not linear), since the latter results in excessive brightening.
		// The palette-matching below ensures that the 50% dithered checkerboard perceptual brightness matches around 64% intensity (rather than 50%).
		vec3 ditheredColorSRGB = sourceColor.rgb + ditherOffset * ditheringIntensity;
		vec3 ditheredColorLinear = sRGBToLinearRGB(ditheredColorSRGB);
		vec3 ditheredColorYUV = linearRGBToYUV(ditheredColorLinear);

		vec4 nearestPaletteColor = vec4(-1);
		float shortestDistance = infinity;
		for (int c = 0; c < paletteCount; ++c)
		{
			vec4 paletteColorSRGB = getPaletteColor(c);
			vec3 paletteColorLinear = sRGBToLinearRGB(paletteColorSRGB.rgb);
			vec3 paletteColorYUV = linearRGBToYUV(paletteColorLinear);
			float distance = distance(paletteColorYUV, ditheredColorYUV);
			if (distance < shortestDistance)
			{
				nearestPaletteColor = paletteColorSRGB;
				if (distance < colorDistanceThreshold)
					break;
				shortestDistance = distance;
			}
		}

		// The framebuffer expects an sRGB premultiplied color; combine alpha from the source image and palette.
		float outputAlpha = nearestPaletteColor.a * sourceColor.a;
		gl_FragColor = vec4(nearestPaletteColor.rgb * outputAlpha, outputAlpha);
	}
);

VuoDitherDiffusionMatrix VuoDitherDiffusionMatrixes[] = {
	[30] = {  // South 1×2
		.steps = {
			{ 0, 1, 1},
			{ 0, 0, 0},
			{ 0, 0, 0},
			{ 0, 0, 0},
		},
		.stepCount = 1,
		.divisor = 1,
	},
	[31] = {  // East 2×1
		.steps = {
			{ 1, 0, 1},
			{ 0, 0, 0},
			{ 0, 0, 0},
			{ 0, 0, 0},
		},
		.stepCount = 1,
		.divisor = 1,
	},
	[32] = {  // South + east 2×2
		.steps = {
			{ 1, 0, 1},
			{ 0, 1, 1},
			{ 0, 0, 0},
			{ 0, 0, 0},
		},
		.stepCount = 2,
		.divisor = 2,
	},
	[33] = {  // South + east + southeast 2×2
		.steps = {
			{ 1, 0, 1},
			{ 0, 1, 1},
			{ 1, 1, 1},
			{ 0, 0, 0},
		},
		.stepCount = 3,
		.divisor = 3,
	},
	[34] = {  // Floyd-Steinberg 3×2 (1975)
		.steps = {
			{ 1, 0, 7},
			{-1, 1, 3},
			{ 0, 1, 5},
			{ 1, 1, 1},
		},
		.stepCount = 4,
		.divisor = 16,
	},
	[35] = {  // False Floyd-Steinberg 2×2
		// http://www.textfiles.com/computers/dhalf.txt
		.steps = {
			{ 1, 0, 3},
			{ 0, 1, 3},
			{ 1, 1, 2},
			{ 0, 0, 0},
		},
		.stepCount = 3,
		.divisor = 8,
	},
	[36] = {  // Jarvis-Judice-Ninke 5×3 (1976)
		// Values from the 1976 paper.
		// https://hbfs.wordpress.com/2013/12/31/dithering/ says "divisor 46" and omits the `1` values; I'm not sure where that variant came from.
		.steps = {
			{ 1, 0, 7},
			{ 2, 0, 5},
			{-2, 1, 3},
			{-1, 1, 5},
			{ 0, 1, 7},
			{ 1, 1, 5},
			{ 2, 1, 3},
			{-2, 2, 1},
			{-1, 2, 3},
			{ 0, 2, 5},
			{ 1, 2, 3},
			{ 2, 2, 1},
		},
		.stepCount = 12,
		.divisor = 48,
	},
	[37] = {  // Stucki 5×3 (1981)
		.steps = {
			{ 1, 0, 8},
			{ 2, 0, 4},
			{-2, 1, 2},
			{-1, 1, 4},
			{ 0, 1, 8},
			{ 1, 1, 4},
			{ 2, 1, 2},
			{-2, 2, 1},
			{-1, 2, 2},
			{ 0, 2, 4},
			{ 1, 2, 2},
			{ 2, 2, 1},
		},
		.stepCount = 12,
		.divisor = 42,
	},
	[38] = {  // Burkes 5×2 (1988)
		.steps = {
			{ 1, 0, 8},
			{ 2, 0, 4},
			{-2, 1, 2},
			{-1, 1, 4},
			{ 0, 1, 8},
			{ 1, 1, 4},
			{ 2, 1, 2},
			{ 0, 0, 0},
		},
		.stepCount = 7,
		.divisor = 32,
	},
	[39] = {  // Atkinson 4×3 (1988)
		.steps = {
			{ 1, 0, 1},
			{ 2, 0, 1},
			{-1, 1, 1},
			{ 0, 1, 1},
			{ 1, 1, 1},
			{ 0, 2, 1},
			{ 0, 0, 0},
			{ 0, 0, 0},
		},
		.stepCount = 6,
		.divisor = 8,
	},
	[40] = {  // Sierra "Three-Line" 5×3 (1989)
		// http://www.textfiles.com/computers/dhalf.txt
		.steps = {
			{ 1, 0, 5},
			{ 2, 0, 3},
			{-2, 1, 2},
			{-1, 1, 4},
			{ 0, 1, 5},
			{ 1, 1, 4},
			{ 2, 1, 2},
			{-1, 2, 2},
			{ 0, 2, 3},
			{ 1, 2, 2},
			{ 0, 0, 0},
			{ 0, 0, 0},
		},
		.stepCount = 10,
		.divisor = 32,
	},
	[41] = {  // Sierra "Two-Line" 5×2 (1990)
		// http://www.textfiles.com/computers/dhalf.txt
		.steps = {
			{ 1, 0, 4},
			{ 2, 0, 3},
			{-2, 1, 1},
			{-1, 1, 2},
			{ 0, 1, 3},
			{ 1, 1, 2},
			{ 2, 1, 1},
			{ 0, 0, 0},
		},
		.stepCount = 7,
		.divisor = 16,
	},
	[42] = {  // Sierra "Filter Lite" 3×2 (1990)
		// http://www.textfiles.com/computers/dhalf.txt
		.steps = {
			{ 1, 0, 2},
			{-1, 1, 1},
			{ 0, 1, 1},
			{ 0, 0, 0},
		},
		.stepCount = 3,
		.divisor = 4,
	},
	[43] = {  // Shiau-Fan 4×2 (1994)
		.steps = {
			{ 1, 0, 4},
			{-2, 1, 1},
			{-1, 1, 1},
			{ 0, 1, 2},
		},
		.stepCount = 4,
		.divisor = 8,
	},
	[44] = {  // Shiau-Fan 5×2 (1994)
		.steps = {
			{ 1, 0, 8},
			{-3, 1, 1},
			{-2, 1, 1},
			{-1, 1, 2},
			{ 0, 1, 4},
			{ 0, 0, 0},
			{ 0, 0, 0},
			{ 0, 0, 0},
		},
		.stepCount = 5,
		.divisor = 16,
	},
	[45] = {  // Pigeon 5×3 (2013)
		// https://hbfs.wordpress.com/2013/12/31/dithering/
		.steps = {
			{ 1, 0, 2},
			{ 2, 0, 1},
			{-1, 1, 2},
			{ 0, 1, 2},
			{ 1, 1, 2},
			{-2, 2, 1},
			{ 0, 2, 1},
			{ 2, 2, 1},
		},
		.stepCount = 8,
		.divisor = 14,
	},
};

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
	VuoErrorDiffusionDither errorDiffusion;

	struct
	{
		VuoInteger ditheringMode;
	} priorSettings;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *instance = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = NULL;

	instance->priorSettings.ditheringMode = -1;

	return instance;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoList_VuoColor, {"default":["#000", "#fff"]}) palette,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":1, "name":"Palette-Matching Accuracy"}) paletteMatchingAccuracy,
	VuoInputData(VuoInteger, {"menuItems":[
		"Noise",
		{"value":  0, "name":"    White Noise"},
		{"value": 10, "name":"    Gradient Noise 0.05"},
		{"value": 11, "name":"    Gradient Noise 0.02"},
		{"value": 12, "name":"    Gradient Noise 0.01"},
		"Ordered",
		{"value": 20, "name":"    Bayer 2×2"},
		{"value": 24, "name":"    Bayer 3×3"},
		{"value": 21, "name":"    Bayer 4×4"},
		{"value": 22, "name":"    Bayer 8×8"},
		{"value": 23, "name":"    Bayer 16×16"},
		"Error Diffusion",
		{"value": 30, "name":"    South"},
		{"value": 31, "name":"    East"},
		{"value": 32, "name":"    South + East"},
		{"value": 33, "name":"    South + East + Southeast"},
		{"value": 34, "name":"    Floyd-Steinberg (1975)"},
		{"value": 35, "name":"    False Floyd-Steinberg"},
		{"value": 36, "name":"    Jarvis-Judice-Ninke (1976)"},
		{"value": 37, "name":"    Stucki (1981)"},
		{"value": 38, "name":"    Burkes (1988)"},
		{"value": 39, "name":"    Atkinson (1988)"},
		{"value": 40, "name":"    Sierra \"Three-Line\" 5×3 (1989)"},
		{"value": 41, "name":"    Sierra \"Two-Line\" 5×2 (1990)"},
		{"value": 42, "name":"    Sierra \"Filter Lite\" 3×2 (1990)"},
		{"value": 43, "name":"    Shiau-Fan 4×2 (1994)"},
		{"value": 44, "name":"    Shiau-Fan 5×2 (1994)"},
		{"value": 45, "name":"    Pigeon (2013)"},
	], "default":23}) dithering,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":1}) ditheringIntensity,
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

		if (dithering >= 20 && dithering <= 24)  // Bayer and noise dithering, implemented with OpenGL/GLSL
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
		else if (dithering >= 30 && dithering <= 45)  // Error diffusion dithering, implemented with Metal/MSL
			if (!(*instance)->errorDiffusion)
			{
				(*instance)->errorDiffusion = VuoErrorDiffusionDither_make();
				VuoRetain((*instance)->errorDiffusion);
			}

		(*instance)->priorSettings.ditheringMode = dithering;
	}

	float colorDistanceThreshold = 2. - pow(VuoReal_clamp(paletteMatchingAccuracy, 0, 1), 0.2) * 2.;

	if (dithering < 30)  // OpenGL/GLSL
	{
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

		VuoShader_setUniform_VuoReal((*instance)->shader, "colorDistanceThreshold", colorDistanceThreshold);
		VuoShader_setUniform_VuoReal((*instance)->shader, "ditheringIntensity", ditheringIntensity);

		*quantizedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
	}

	else if (dithering >= 30 && dithering <= 45)  // Metal/MSL
	{
		json_object *imageJson = VuoImage_getInterprocessJson(image);
		json_object *imageId;
		json_object_object_get_ex(imageJson, "ioSurface", &imageId);
		IOSurfaceRef imageSurf = IOSurfaceLookup(json_object_get_int(imageId));

		IOSurfaceRef outputImageSurf = VuoErrorDiffusionDither_processImage((*instance)->errorDiffusion, imageSurf, palette, colorDistanceThreshold, VuoDitherDiffusionMatrixes[dithering], ditheringIntensity);

		VuoIoSurfacePool_signal(imageSurf);
		CFRelease(imageSurf);
		json_object_put(imageJson);

		if (!outputImageSurf)
		{
			*quantizedImage = NULL;
			return;
		}

		json_object *outputImageJson = json_object_new_object();
		json_object_object_add(outputImageJson, "ioSurface", json_object_new_int(IOSurfaceGetID(outputImageSurf)));
		json_object_object_add(outputImageJson, "pixelsWide", json_object_new_int64(IOSurfaceGetWidth(outputImageSurf)));
		json_object_object_add(outputImageJson, "pixelsHigh", json_object_new_int64(IOSurfaceGetHeight(outputImageSurf)));
		*quantizedImage = VuoImage_makeFromJson(outputImageJson);
		json_object_put(outputImageJson);
		CFRelease(outputImageSurf);
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->errorDiffusion);
}
