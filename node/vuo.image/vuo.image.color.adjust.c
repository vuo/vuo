/**
 * @file
 * vuo.image.color.adjust node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>
#include "VuoShader.h"

VuoModuleMetadata({
					 "title" : "Adjust Image Colors",
					 "keywords" : [ "saturation", "desaturate", "grayscale", "greyscale", "tint", "tone", "chroma", "brightness", "contrast", "gamma", "exposure", "filter" ],
					 "version" : "1.2.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(hsl)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D image;

	uniform float saturation;
	uniform float brightness;
	uniform float contrast;
	uniform float gamma;
	uniform float exposure;
	uniform float hueShift;

	void main(void)
	{
		// http://stackoverflow.com/questions/944713/help-with-pixel-shader-effect-for-brightness-and-contrast
		vec4 pixelColor = texture2D(image, fragmentTextureCoordinate.xy);
		pixelColor.rgb /= pixelColor.a;

		// compensate for channels that are completely void - this allows a fully exposed pixel
		// to always be white, regardless of starting value. .001 is the minimum resolution that
		// enables this.
		if( exposure > 0.)
			pixelColor.rgb += vec3(.001);

		// Apply Exposure
		pixelColor.rgb = clamp(pixelColor.rgb * pow(2., exposure), 0., 1.);

		vec3 hsl = rgbToHsl(pixelColor.rgb);

		// Apply hue shift
		hsl.x = mod(hsl.x + hueShift, 1.);

		// // Apply saturation
		hsl.y *= saturation;

		pixelColor.rgb = hslToRgb(hsl);

		// Apply contrast.
		pixelColor.rgb = ((pixelColor.rgb - 0.5f) * max(contrast, 0.)) + 0.5f;

		// Apply brightness.
		pixelColor.rgb += brightness;
		pixelColor.rgb = clamp(pixelColor.rgb, 0., 1.);

		// // Apply gamma.
		pixelColor.r = pow(pixelColor.r, gamma);
		pixelColor.g = pow(pixelColor.g, gamma);
		pixelColor.b = pow(pixelColor.b, gamma);


		// Return final pixel color.
		pixelColor.rgb *= pixelColor.a;

		gl_FragColor = pixelColor;
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

	instance->shader = VuoShader_make("Adjust Image Colors");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

// saturation (0 to 2)
// brightness (-1 to 1)
// contrast (0 to 2)
// gamma (0 to 3)
// exposure (-10 to 10)
void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,

		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":2, "suggestedStep":0.1}) saturation,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-180, "suggestedMax":180, "suggestedStep":1}) hueShift,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) contrast,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) brightness,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) exposure,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":3, "suggestedStep":0.1}) gamma,
		VuoOutputData(VuoImage) adjustedImage
)
{
	if(!image)
		return;

	VuoShader_setUniform_VuoImage((*instance)->shader, "image", image);

	// *scaledValue = (value - start) * scaledRange / range + scaledStart;
	VuoShader_setUniform_VuoReal((*instance)->shader, "saturation", MAX(0,saturation+1)); 	// 0 - 3 values
	VuoShader_setUniform_VuoReal((*instance)->shader, "hueShift", hueShift/360.);
	VuoShader_setUniform_VuoReal((*instance)->shader, "brightness", brightness);	// -1, 1 values
	VuoShader_setUniform_VuoReal((*instance)->shader, "contrast", contrast+1);		// 0, 2 values
	VuoShader_setUniform_VuoReal((*instance)->shader, "gamma", gamma);
	VuoShader_setUniform_VuoReal((*instance)->shader, "exposure", exposure*10);

	// Render.
	*adjustedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
