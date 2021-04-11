/**
 * @file
 * vuo.image.vignette node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Vignette Image",
					  "keywords" : [
						  "filter",
						  "border", "surround", "encapsulate", "darken", "post-process",
						  "circle", "oval", "soften", "fade", "edge", "old", "daguerreotype", "vintage", ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "VignetteImage.vuo" ]
					  }
				 });

static const char * vignetteFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 center;
	uniform vec2 size;
	uniform float blur;
	uniform vec4 edgeColor;
	uniform bool replaceOpacity;

	float smootherstep(float edge0, float edge1, float x)
	{
		x = clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
		return x*x*x*(x*(x*6 - 15) + 10);
	}

	void main(void)
	{
		vec4 col = texture2D(texture, fragmentTextureCoordinate);
		col.a = clamp(col.a, 0., 1.);	// for floating-point textures

		vec2 uv = (fragmentTextureCoordinate - center) / size;
		float f = smootherstep(.5 + blur, .5 - blur, length(uv));
		vec3 mixed = mix(edgeColor.rgb, col.rgb, f);
		float a = mix(edgeColor.a, col.a, f);

		vec4 mixed2 = replaceOpacity ? vec4(mixed,a) : mix(col, vec4(mixed, a), edgeColor.a);
		gl_FragColor = mixed2;
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

	instance->shader = VuoShader_make("Vignette Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, vignetteFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoColor, {"default":{"r":0,"g":0,"b":0,"a":1}}) color,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0, "suggestedMax":4, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0, "suggestedMax":4, "suggestedStep":0.1}) height,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) sharpness,
		VuoInputData(VuoBoolean, {"default":true}) replaceOpacity,
		VuoOutputData(VuoImage) vignettedImage
)
{
	if (!image)
	{
		*vignettedImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));

	float clampedWidth  = MAX(0, width );
	float clampedHeight = MAX(0, height);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "size", (VuoPoint2d){
										VuoShader_samplerSizeFromVuoSize(clampedWidth ),
										VuoShader_samplerSizeFromVuoSize(clampedHeight) * ((float)image->pixelsWide / image->pixelsHigh) });

	float blur = (1. - VuoReal_clamp(sharpness, 0, 1)) / 4.;
	blur = MAX(blur, (1./MIN(clampedWidth,clampedHeight))/image->pixelsWide);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "blur", blur);

	VuoShader_setUniform_VuoColor  ((*instance)->shader, "edgeColor", color);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "replaceOpacity", replaceOpacity);

	*vignettedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
