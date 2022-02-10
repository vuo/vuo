/**
 * @file
 * vuo.image.make.noise.sphere node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoImageNoise.h"
#include "VuoGradientNoise.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Spherical Noise Image",
					  "keywords" : [
						  "equirectangular", "mercator", "seamless",
						  "perlin", "simplex", "gradient",
						  "value", "random",
						  "fractal", "fractional Brownian noise", "fBm",
						  "octaves", "persistence", "lacunarity",
					  ],
					  "version" : "1.1.1",
					  "node": {
						  "exampleCompositions" : [ "SpinWaterySphere.vuo", "CompareNoiseTypes.vuo",
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

	varying vec2 fragmentTextureCoordinate;

	\n#include "GPUNoiseLib.glsl"
	\n#include "noise3D.glsl"

	// Preprocessor statements below are prefixed by `\n` so that the C preprocessor ignores them,
	// preserving them for use by the GLSL preprocessor.  (https://stackoverflow.com/a/17542260)
	// And they're suffixed by `\n` since stringification removes linebreaks,
	// which preprocessor statements require.

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
		vec2 p = fragmentTextureCoordinate;

		// Treat `p` as a spherical coordinate.
		p *= vec2(2 * 3.1415926535, 3.1415926535);

		// Convert to cartesian.
		vec3 noiseCoordinate = vec3(
			sin(p.y)*cos(p.x),
			sin(p.y)*sin(p.x),
			cos(p.y));

		noiseCoordinate *= scale;
		noiseCoordinate -= center;

		float intensity = 0.;
		float amplitude = 1.;
		float mixSum = 0.;
		for (int i = 0; i < levels; ++i)
		{
			vec3 nc = noiseCoordinate;

			// Scale to the current octave.
			nc.xyz *= pow(spacing, float(i));

			// Start each octave in a different position, so we don't see streaks when all octaves approach (0,0,0).
			nc.z += float(i);

			intensity += NOISE;

			mixSum += amplitude;
			amplitude *= roughness;
		}

		float noise =
		\n#if RANGE_MODE == 0\n // None
			(RESOLVE - range.x) / (range.y - range.x);
		\n#elif RANGE_MODE == 1\n // Clamp
			(clamp(RESOLVE, range.x, range.y) - range.x) / (range.y - range.x);
		\n#elif RANGE_MODE == 2\n // Repeat
			mod((RESOLVE - range.x) / (range.y - range.x), 1.);
		\n#else\n // Mirrored Repeat
//			1. - abs(mod((RESOLVE - range.x) / (range.y - range.x), 2.) - 1.); // triangle
			.5 + sin( (RESOLVE - range.x - .5) / (range.y - range.x) * 3.141592 ) / 2.;
		\n#endif\n

		gl_FragColor = mix(colorA, colorB, noise);
	}
);

struct nodeInstanceData
{
	VuoShader shader;

	struct
	{
		VuoImageNoise type;
		VuoGradientNoise grid;
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
	instance->priorSettings.rangeMode = -1;

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) colorA,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) colorB,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMin":{"x":-10.0,"y":-10.0,"z":-10.0}, "suggestedMax":{"x":10.0,"y":10.0,"z":10.0}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0.001, "suggestedMax":1., "suggestedStep":0.1}) scale,
		VuoInputData(VuoImageNoise, {"default":"gradient"}) type,
		VuoInputData(VuoGradientNoise, {"default":"triangular"}) grid,
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
		VuoOutputData(VuoImage) image
)
{
	if ((*instance)->priorSettings.type != type
	 || (*instance)->priorSettings.grid != grid
	 || (*instance)->priorSettings.rangeMode != rangeMode)
	{
		VuoRelease((*instance)->shader);

		char *sourceWithPrefix = VuoText_format("#version 120\n#define TYPE %d\n#define GRID %d\n#define RANGE_MODE %d\n\n%s", type, grid, rangeMode, fragmentShaderSource);

		(*instance)->shader = VuoShader_make("Spherical Noise Shader");
		VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, sourceWithPrefix);
		VuoRetain((*instance)->shader);

		(*instance)->priorSettings.type = type;
		(*instance)->priorSettings.grid = grid;
		(*instance)->priorSettings.rangeMode = rangeMode;
	}

	bool rangeInverted = VuoRange_isInverted(range);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "colorA", rangeInverted ? colorB : colorA);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "colorB", rangeInverted ? colorA : colorB);

	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center", VuoPoint3d_make(-center.z/2., -center.x/2., -center.y/2.));

	// Multiply scale by 2*pi so it matches the perceptual scale of the `Make (non-spherical) Noise Image` node.
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale",  1./VuoReal_makeNonzero(scale * 2 * M_PI));

	VuoRange r = VuoRange_getOrderedRange(range);
	// Limit sharpness to output resolution, to reduce aliasing.
	r.minimum -= 2./width;
	r.maximum += 2./width;
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "range",  (VuoPoint2d){r.minimum, r.maximum});

	VuoShader_setUniform_VuoInteger((*instance)->shader, "levels", MAX(1, levels));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "roughness", roughness);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "spacing", VuoReal_makeNonzero(MAX(0, spacing)));

	// Render.
	*image = VuoImageRenderer_render((*instance)->shader, width, width/2, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
