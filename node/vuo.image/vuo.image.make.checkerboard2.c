/**
 * @file
 * vuo.image.make.checkerboard node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoGridType.h"

VuoModuleMetadata({
					  "title" : "Make Checkerboard Image",
					  "keywords" : [ "backdrop", "background", "checkers", "chess", "debug", "troubleshoot", "uvw", "mosaic",
						  "stripes", "lines", "bars", "horizontal", "vertical", "pattern"
					  ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "ExploreCheckerboard.vuo" ]
					  }
				 });

static const char * checkerboardFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	varying vec4 fragmentTextureCoordinate;

	uniform vec4 color1;
	uniform vec4 color2;
	uniform float sharpness;
	uniform float crossfadeAmount;
	uniform float crossfadeCoverage;
	uniform vec4 rotationMatrix;
	uniform vec2 center;
	uniform vec2 coverage;
	uniform vec2 imageSize;

	void main()
	{
		// Based on the Gritz/Baldwin antialiased checkerboard shader.

		vec2 c = fragmentTextureCoordinate.xy - center;
		c *= mat2(rotationMatrix.x, rotationMatrix.y, rotationMatrix.z, rotationMatrix.w);
		vec2 filterWidth = (fwidth(c) + sharpness);

		vec2 checkPos = fract(c + filterWidth/2);
		vec2 p = smoothstep(coverage,  filterWidth + coverage,  checkPos) +
			(1 - smoothstep(vec2(0),   filterWidth,             checkPos));

		// Hide the antialiasing artifacts.
		if (coverage.x == 1) p.x = 0.;
		if (coverage.y == 1) p.y = 0.;

		vec4 mixed = mix(color1, color2, p.x*p.y + (1-p.x)*(1-p.y));
		gl_FragColor = mix(mixed, mix(color1,color2,crossfadeCoverage), crossfadeAmount);
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
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) upperLeftColor,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) upperRightColor,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.01}) squareSize,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoInputData(VuoReal, {"default":0., "suggestedMin":0., "suggestedMax":360.}) angle,
		VuoInputData(VuoPoint2d, {"default":{"x":0.5,"y":0.5}, "suggestedMin":{"x":0.0,"y":0.0}, "suggestedMax":{"x":1.0,"y":1.0}, "suggestedStep":{"x":0.1,"y":0.1}}) coverage,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	double aspect = (float)width / (float)height;
	const double minSquareSize = 2./width;
	double clampedSquareSize = MAX(squareSize, minSquareSize);

	// When the squares get really small, crossfade to a solid color, to hide the aliasing/moiré.
	const double crossfadeSquareSize = 6./width;
	double crossfadeAmount = 0.;
	if (clampedSquareSize < crossfadeSquareSize)
		crossfadeAmount = VuoReal_clamp(1. - (clampedSquareSize - minSquareSize) / (crossfadeSquareSize - minSquareSize), 0, 1);

	VuoPoint2d cen = { (center.x+1)/2., (center.y*aspect + 1)/2. };

	double sharpnessMin = MAX(fabs(coverage.x - .5)*2., fabs(coverage.y - .5)*2.);

	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", cen);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "coverage", VuoPoint2d_clamp(coverage, 0, 1));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "crossfadeCoverage", coverage.x*coverage.y + (1.-coverage.x)*(1.-coverage.y));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "sharpness", VuoReal_clamp(.5 - MAX(sharpness,sharpnessMin)/2., 0, .5));
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "color1", upperLeftColor);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "color2", upperRightColor);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "crossfadeAmount", crossfadeAmount);

	float cs = sin(angle * M_PI/180.);
	float cc = cos(angle * M_PI/180.);
	VuoPoint4d rotationMatrix;
	rotationMatrix.x =  cc /  clampedSquareSize;
	rotationMatrix.y =  cs / (clampedSquareSize * aspect);
	rotationMatrix.z = -cs /  clampedSquareSize;
	rotationMatrix.w =  cc / (clampedSquareSize * aspect);
	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "rotationMatrix", rotationMatrix);

	// Render.
	*image = VuoImageRenderer_render((*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
