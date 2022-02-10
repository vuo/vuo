/**
 * @file
 * vuo.image.make.stripe node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoGridType.h"

VuoModuleMetadata({
					  "title" : "Make Stripe Image",
					  "keywords" : [
						  "mosaic", "stripes", "lines", "bars", "horizontal", "vertical", "pattern"
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "AnimateStripeWidth.vuo" ]
					  }
				 });

static const char * checkerboardFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	varying vec2 fragmentTextureCoordinate;

	uniform vec4 color1;
	uniform vec4 color2;
	uniform float sharpness;
	uniform float crossfadeAmount;
	uniform mat2 rotationMatrix;
	uniform vec2 gridType;
	uniform vec2 center;
	uniform float coverage;
	uniform vec2 imageSize;

	void main()
	{
		// Based on the Gritz/Baldwin antialiased checkerboard shader.

		vec2 c = fragmentTextureCoordinate - center;
		c *= rotationMatrix;
		float filterWidth = (fwidth(c.x) + sharpness);

		float checkPos = fract(c.x + filterWidth/2);
		float p = smoothstep(coverage,  filterWidth + coverage,  checkPos) +
			 (1 - smoothstep(0,         filterWidth,             checkPos));

		// Hide the antialiasing artifacts.
		if (coverage == 1) p = 0.;

		vec4 mixed = mix(color1, color2, p);
		gl_FragColor = mix(mixed, mix(color1,color2,1.-coverage), crossfadeAmount);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Checkerboard Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, checkerboardFragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) colorA,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) colorB,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.01}) stripeWidth,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoInputData(VuoReal, {"default":0., "suggestedMin":0., "suggestedMax":360.}) angle,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1}) coverage,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	double aspect = (float)width / (float)height;
	const double minStripeWidth = 2./width;
	double clampedStripeWidth = MAX(stripeWidth, minStripeWidth);

	// When the squares get really small, crossfade to a solid color, to hide the aliasing/moiré.
	const double crossfadeSquareSize = 6./width;
	double crossfadeAmount = 0.;
	if (clampedStripeWidth < crossfadeSquareSize)
		crossfadeAmount = VuoReal_clamp(1. - (clampedStripeWidth - minStripeWidth) / (crossfadeSquareSize - minStripeWidth), 0, 1);

	VuoPoint2d cen = { (center.x+1)/2., (center.y*aspect + 1)/2. };

	double sharpnessMin = fabs(coverage - .5)*2.;

	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", cen);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "coverage", coverage);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "sharpness", VuoReal_clamp(.5 - MAX(sharpness,sharpnessMin)/2., 0, .5));
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "color1", colorA);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "color2", colorB);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "crossfadeAmount", crossfadeAmount);

	float cs = sin(angle * M_PI/180.);
	float cc = cos(angle * M_PI/180.);
	float *rotationMatrix = (float *)malloc(sizeof(float) * 4);
	VuoRegister(rotationMatrix, free);
	rotationMatrix[0] =  cc /  clampedStripeWidth;
	rotationMatrix[1] =  cs / (clampedStripeWidth * aspect);
	rotationMatrix[2] = -cs /  clampedStripeWidth;
	rotationMatrix[3] =  cc / (clampedStripeWidth * aspect);
	VuoShader_setUniform_mat2((*instance)->shader, "rotationMatrix", rotationMatrix);

	// Render.
	*image = VuoImageRenderer_render((*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
