/**
 * @file
 * vuo.image.dilate node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"
#include "VuoBlurShape.h"
#include "VuoThresholdType.h"

VuoModuleMetadata({
	"title" : "Dilate Image",
	"keywords" : [
		"bolden", "lighten", "grow", "expand", "enlarge", "maximum", "patch holes", "fill gaps", "open",
		"erode", "darken", "shrink", "contract", "minimum", "close", "reduce",
		"noise reduction", "simplify", "smooth",
		"morphological", "morphology",
		"pointillism", "impressionism",
		"mask",
		"filter"
	],
	"version" : "1.1.0",
	"dependencies" : [
		"VuoImageRenderer"
	],
	"node": {
		"exampleCompositions" : [ "DilateImage.vuo" ]
	}
});

static const char * fragmentShaderSource = VUO_STRINGIFY(
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


struct nodeInstanceData
{
	VuoShader shader[2][4]; // [func][shape]
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	for (int func = 0; func <= 1; ++func)
		for (int shape = VuoBlurShape_Box; shape <= VuoBlurShape_Disc; ++shape)
		{
			char *sourceWithPrefix = VuoText_format("#version 120\n#define FUNC %d\n#define SHAPE %d\n\n%s", func, shape, fragmentShaderSource);
			instance->shader[func][shape] = VuoShader_make("Morphology Shader");
			VuoShader_addSource(instance->shader[func][shape], VuoMesh_IndividualTriangles, NULL, NULL, sourceWithPrefix);
			VuoRetain(instance->shader[func][shape]);
		}

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoBlurShape, {"default":"disc", "includeValues":["box","disc"]}) shape,
	VuoInputData(VuoReal, {"default":4.0,"suggestedMin":-10.0,"suggestedMax":10.0,"suggestedStep":1.0}) radius,
	VuoInputData(VuoBoolean, {"default":false}) rescind,
	VuoOutputData(VuoImage) dilatedImage
)
{
	if (! image)
	{
		*dilatedImage = NULL;
		return;
	}

	int func = radius >= 0 ? 0 : 1;
	int shape2 = MAX(MIN(shape, VuoBlurShape_Disc), VuoBlurShape_Box);

	float scaledRadius = radius * image->scaleFactor;

	VuoShader_setUniform_VuoImage((*instance)->shader[func][shape2], "texture", image);
	VuoShader_setUniform_VuoReal((*instance)->shader[func][shape2], "halfWidth", fabs(scaledRadius) + .5);

	*dilatedImage = VuoImageRenderer_render((*instance)->shader[func][shape2], image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	if (rescind)
	{
		VuoImage d = *dilatedImage;
		VuoLocal(d);

		int otherFunc = 1-func;
		VuoShader_setUniform_VuoImage((*instance)->shader[otherFunc][shape2], "texture", d);
		VuoShader_setUniform_VuoReal((*instance)->shader[otherFunc][shape2], "halfWidth", fabs(scaledRadius) + .5);

		*dilatedImage = VuoImageRenderer_render((*instance)->shader[otherFunc][shape2], image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	for (int func = 0; func <= 1; ++func)
		for (int shape = VuoBlurShape_Box; shape <= VuoBlurShape_Disc; ++shape)
			VuoRelease((*instance)->shader[func][shape]);
}
