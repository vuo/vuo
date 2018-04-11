/**
 * @file
 * vuo.image.scramble node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Scramble Image",
					 "keywords" : [
						 "filter",
						 "rearrange", "shuffle", "mosaic", "censor",
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoImageRenderer"
					 ],
					 "node": {
						  "exampleCompositions" : [ "UnscrambleImage.vuo" ]
					 }
				 });

static const char * pixelFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)
	include(noise3D)

	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 viewportSize;
	uniform vec2 squareSize;
	uniform vec2 center;
	uniform float chaos;
	uniform float time;

	void main(void)
	{
		vec2 noisePos = fragmentTextureCoordinate.xy;

		// Quantize the texture coordinate so it lands exactly on an output pixel,
		// since mod()ding near a big pixel boundary
		// can exaggerate the GPU's imprecise floating point math.
		noisePos = vec2(int(noisePos.x*viewportSize.x)/viewportSize.x,
						int(noisePos.y*viewportSize.y)/viewportSize.y);

		vec2 centerOffset = mod(center - squareSize/2., squareSize);
		vec2 distanceFromCorner = mod(noisePos - centerOffset, squareSize);
		noisePos = noisePos - distanceFromCorner + squareSize/2.;

		noisePos = (noisePos - center) / (1.5 * squareSize);
		vec3 noise3 = vec3(.5) + snoise3D3D(vec3(noisePos.x, noisePos.y, time)) / 2.;
		noisePos = noise3.xy;

		vec2 samplePos;
		if (noise3.z <= chaos)
		{
			// Quantize to squareSize
//			noisePos = floor(noisePos/squareSize)*squareSize;

			// Shift so we don't sample beyond the image.
			noisePos = clamp(noisePos, vec2(0.), 1. - squareSize);

			samplePos = noisePos + distanceFromCorner;
		}
		else
			samplePos = fragmentTextureCoordinate.xy;

		gl_FragColor = VuoGlsl_sample(texture, samplePos);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Scramble Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, pixelFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.05}) squareSize,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.2,"suggestedMin":0.0,"suggestedMax":1.0}) chaos,
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoOutputData(VuoImage) scrambledImage
)
{
	if (!image)
	{
		*scrambledImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoReal clampedSquareSize = VuoReal_clamp(squareSize, 2./w, MIN(2, 2.*h/w));
	VuoPoint2d squareSize2d = VuoPoint2d_multiply(VuoPoint2d_make(1., (float)w/h), VuoShader_samplerSizeFromVuoSize(clampedSquareSize));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "squareSize",  squareSize2d);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center",      VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "chaos",       VuoReal_clamp(chaos, 0, 1));

	// Offset by .5 so that when Time is 0 and constant (the default), the full range of the Chaos slider is useful.
	// (Without this, a bunch of squares suddenly appear when the Chaos slider reaches about 0.12.)
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "time",        time+.5);

	*scrambledImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
