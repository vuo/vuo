/**
 * @file
 * vuo.image.make.noise node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoImageNoise.h"
#include "VuoGradientNoise.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Noise Image",
					  "keywords" : [
						  "perlin", "simplex", "gradient",
						  "cellular", "Worley", "Voronoi", "point",
						  "value", "random",
						  "polka", "dots", "circles", "ovals",
						  "fractal", "fractional Brownian noise", "fBm",
						  "octaves", "persistence", "lacunarity",
					  ],
					  "version" : "1.2.0",
					  "node": {
						  "exampleCompositions" : [ "ShowNoiseImage.vuo", "CompareNoiseTypes.vuo",
									    "CompareNoiseRangeModes.vuo" ]
					  }
				 });

static const char *fragmentShaderSource = VUO_STRINGIFY(
	uniform vec4 colorA;
	uniform vec4 colorB;
	uniform vec3 center;
	uniform float scale;
	uniform vec2 range;  // x=min, y=max
	uniform int levels;
	uniform float roughness;
	uniform float spacing;
	uniform float aspectRatio;

	varying vec4 fragmentTextureCoordinate;

	include(GPUNoiseLib)
	include(noise3D)

	// Preprocessor statements below are prefixed by `\n` so that the C preprocessor ignores them,
	// preserving them for use by the GLSL preprocessor.  (http://stackoverflow.com/a/17542260)
	// And they're suffixed by `\n` since stringification removes linebreaks,
	// which preprocessor statements require.

	\n#define ZOFFSET 0.\n

	\n#if TYPE == 0 // Gradient\n
		\n#if GRID == 0 // Rectangular\n
			\n#define NOISE   cnoise3D1D(nc) * amplitude\n
			\n#define RESOLVE (intensity / mixSum) / 2. + .5\n
		\n#else // Triangular\n
			\n#define NOISE   snoise3D1D(nc) * amplitude\n
			\n#define RESOLVE (intensity / mixSum) / 2. + .5\n
		\n#endif\n
	\n#elif TYPE == 1 // Value\n
		\n#if GRID == 0 // Rectangular\n
			\n#define NOISE   Value3D(nc) * amplitude\n
			\n#define RESOLVE intensity / mixSum\n
		\n#else // Triangular\n
			\n#define NOISE   SimplexValue3D(nc) * amplitude\n
			\n#define RESOLVE pow((intensity / mixSum) / 2. + .5, .426)\n
		\n#endif\n
	\n#elif TYPE == 2 // Cellular\n
		\n#if GRID == 0 // Rectangular\n
			\n#define NOISE   Cellular3D(nc) * amplitude\n
			\n#define RESOLVE pow((intensity / mixSum) * 2., .55)\n
		\n#else // Triangular\n
			\n#define NOISE   SimplexCellular3D(nc) * amplitude\n
			\n#define RESOLVE pow((intensity / mixSum) * 1.47, .83)\n
		\n#endif\n
	\n#else // Dot\n
		\n#if GRID == 0 // Rectangular\n
			\n#undef ZOFFSET\n
			\n#define ZOFFSET .5\n
			\n#define NOISE   PolkaDot3D(nc, 0., 1.) * amplitude\n
			\n#define RESOLVE intensity\n
		\n#else // Triangular\n
			\n#define NOISE   SimplexPolkaDot3D(nc, 1., 1.) * amplitude\n
			\n#define RESOLVE intensity\n
		\n#endif\n
	\n#endif\n

	void main()
	{
		vec2 p = fragmentTextureCoordinate.xy;

	\n#if TILE == 1\n
		// https://web.archive.org/web/20150607183420/http://webstaff.itn.liu.se/~stegu/TNM022-2005/perlinnoiselinks/perlin-noise-math-faq.html#tile
		vec2   positions[4] = vec2[]( vec2(p.x,p.y), vec2(p.x-1.,p.y), vec2(p.x-1.,p.y-1.), vec2(p.x,p.y-1.) );
		float amplitudes[4] = float[]( (1.-p.x)*(1.-p.y), p.x*(1.-p.y), p.x*p.y, (1.-p.x)*p.y );
		float noise = 0.;
		for (int i = 0; i < 4; ++i)
		{
			p = positions[i];
	\n#endif\n

			vec3 noiseCoordinate = vec3(p.x - .5, (p.y - .5) / aspectRatio, ZOFFSET);
			noiseCoordinate *= scale;
			noiseCoordinate -= center;

			float intensity = 0.;
			float amplitude = 1.;
			float mixSum = 0.;
			for (int i = 0; i < levels; ++i)
			{
				vec3 nc = noiseCoordinate;

				// Scale to the current octave.
				nc.xy *= pow(spacing, float(i));

				// Start each octave in a different position, so we don't see streaks when all octaves approach (0,0,0).
				nc.z += float(i);

				intensity += NOISE;

				mixSum += amplitude;
				amplitude *= roughness;
			}

			float thisNoiseLevel =
			\n#if RANGE_MODE == 0\n // None
				(RESOLVE - range.x) / (range.y - range.x);
			\n#elif RANGE_MODE == 1\n // Clamp
				(clamp(RESOLVE, range.x, range.y) - range.x) / (range.y - range.x);
			\n#elif RANGE_MODE == 2\n // Repeat
				mod((RESOLVE - range.x) / (range.y - range.x), 1.);
			\n#else\n // Mirrored Repeat
//				1. - abs(mod((RESOLVE - range.x) / (range.y - range.x), 2.) - 1.); // triangle
				.5 + sin( (RESOLVE - range.x - .5) / (range.y - range.x) * 3.141592 ) / 2.;
			\n#endif\n

			// If the edge is very sharp, soften it (reduce aliasing).
			float fuzz = abs(dFdx(thisNoiseLevel)) + abs(dFdy(thisNoiseLevel));
			thisNoiseLevel += fuzz/3.;

	\n#if TILE == 1\n
			noise += thisNoiseLevel * amplitudes[i];
		}
	\n#else\n
		float noise = thisNoiseLevel;
	\n#endif\n

		gl_FragColor = clamp(mix(colorA, colorB, noise), 0., 1.);
	}
);

struct nodeInstanceData
{
	VuoShader shader;

	struct
	{
		VuoImageNoise type;
		VuoGradientNoise grid;
		VuoBoolean tile;
		VuoImageWrapMode rangeMode;
	} priorSettings;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = NULL;

	instance->priorSettings.type = -1;
	instance->priorSettings.grid = -1;
	instance->priorSettings.tile = -1;
	instance->priorSettings.rangeMode = -1;

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) colorA,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) colorB,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-10.0,"y":-10.0}, "suggestedMax":{"x":10.0,"y":10.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal) time,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0.001, "suggestedMax":1., "suggestedStep":0.1}) scale,
		VuoInputData(VuoImageNoise, {"default":"gradient"}) type,
		VuoInputData(VuoGradientNoise, {"default":"triangular"}) grid,
		VuoInputData(VuoBoolean, {"default":false}) tile,
		VuoInputData(VuoRange, {"default":{"minimum":0.0,"maximum":1.0},
								"requireMin":true,
								"requireMax":true,
								"suggestedMin":{"minimum":0.0,"maximum":0.0},
								"suggestedMax":{"minimum":1.0,"maximum":1.0},
								"suggestedStep":{"minimum":0.1,"maximum":0.1}}) range,
		VuoInputData(VuoImageWrapMode, {"default":"clamp"}) rangeMode,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":4, "suggestedStep":1}) levels,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) roughness,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":1.0, "suggestedMax":5.0, "suggestedStep":0.1}) spacing,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	if ((*instance)->priorSettings.type != type
	 || (*instance)->priorSettings.grid != grid
	 || (*instance)->priorSettings.tile != tile
	 || (*instance)->priorSettings.rangeMode != rangeMode)
	{
		VuoRelease((*instance)->shader);

		char *sourceWithPrefix = VuoText_format("#version 120\n#define TYPE %d\n#define GRID %d\n#define TILE %lu\n#define RANGE_MODE %d\n\n%s", type, grid, tile, rangeMode, fragmentShaderSource);

		(*instance)->shader = VuoShader_make("Noise Shader");
		VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, sourceWithPrefix);
		VuoRetain((*instance)->shader);

		(*instance)->priorSettings.type = type;
		(*instance)->priorSettings.grid = grid;
		(*instance)->priorSettings.tile = tile;
		(*instance)->priorSettings.rangeMode = rangeMode;
	}

	bool rangeInverted = VuoRange_isInverted(range);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "colorA", rangeInverted ? colorB : colorA);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "colorB", rangeInverted ? colorA : colorB);

	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center", VuoPoint3d_make(center.x/2., center.y/2., time));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale",  1./VuoReal_makeNonzero(scale));

	VuoRange r = VuoRange_makeNonzero(VuoRange_getOrderedRange(range));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "range",  (VuoPoint2d){r.minimum, r.maximum});

	VuoShader_setUniform_VuoInteger((*instance)->shader, "levels", MAX(1, levels));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "roughness", roughness);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "spacing", spacing);

	// Render.
	*image = VuoImageRenderer_render((*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
