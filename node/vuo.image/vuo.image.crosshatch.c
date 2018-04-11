/**
 * @file
 * vuo.image.crosshatch node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Make Crosshatch Image",
					 "keywords" : [
						 "illustration", "hand-drawn", "sketch", "line art", "stroke", "contour", "woodcut",
						 "plaid", "tartan",
						 "filter"
					 ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * pixelFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 viewportSize;
	uniform float aspectRatio;
	uniform float crossHatchSpacing;
	uniform float lineWidth;
	uniform float threshold;
	uniform vec4 lineColor;
	uniform vec4 backgroundColor;
	uniform vec2 center;

	const vec3 W = vec3(0.2125, 0.7154, 0.0721);

	float smoothLine(float p)
	{
		float fw = 2./viewportSize.x;
		float m = mod(p, crossHatchSpacing);
		return smoothstep(lineWidth - fw, lineWidth, m)
			+ (1. - smoothstep(0., fw, m));
	}

	void main(void)
	{
		vec2 tc = fragmentTextureCoordinate.xy;

		float luminance = dot(VuoGlsl_sample(texture, tc).rgb, W);

		tc -= center;
		tc.y /= aspectRatio;

		float darkness = 1. - luminance;
		float lines = 0.;
		float fw = fwidth(luminance)/2.;

		lines += smoothstep( threshold      - fw,  threshold      + fw, darkness) * smoothLine(tc.x + tc.y                       ); // downward slope, even
		lines += smoothstep((threshold*.75) - fw, (threshold*.75) + fw, darkness) * smoothLine(tc.x - tc.y                       ); //   upward slope, even
		lines += smoothstep((threshold*.5 ) - fw, (threshold*.5 ) + fw, darkness) * smoothLine(tc.x + tc.y - crossHatchSpacing/2.); // downward slope, odd
		lines += smoothstep((threshold*.3 ) - fw, (threshold*.3 ) + fw, darkness) * smoothLine(tc.x - tc.y - crossHatchSpacing/2.); //   upward slope, odd

		gl_FragColor = mix(backgroundColor, lineColor, lines);
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

	instance->shader = VuoShader_make("Crosshatch");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, pixelFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) backgroundColor,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) lineColor,
		VuoInputData(VuoReal, {"default":0.4,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.01}) threshold,
		VuoInputData(VuoReal, {"default":0.03,"suggestedMin":0,"suggestedMax":0.2,"suggestedStep":0.01}) width,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.3,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.01}) coverage,
		VuoOutputData(VuoImage) crosshatchedImage
)
{
	if (!image)
	{
		*crosshatchedImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoColor((*instance)->shader, "backgroundColor", backgroundColor);
	VuoShader_setUniform_VuoColor((*instance)->shader, "lineColor", lineColor);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "crossHatchSpacing", width);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "lineWidth", VuoReal_clamp(width * (1. - coverage), 2./w, width));
	VuoShader_setUniform_VuoReal ((*instance)->shader, "threshold", (threshold - .1) * 4.);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoPoint2d_add(VuoShader_samplerCoordinatesFromVuoCoordinates(center, image),
																				  (VuoPoint2d){VuoReal_clamp(coverage, 0, 1)*width/2., 0}));

	*crosshatchedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
