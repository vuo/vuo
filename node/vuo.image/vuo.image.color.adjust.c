/**
 * @file
 * vuo.image.color.adjust node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>
#include "VuoShader.h"

VuoModuleMetadata({
					 "title" : "Adjust Image Colors",
					 "keywords" : [
						 "saturation", "desaturate",
						 "grayscale", "greyscale",
						 "tone", "chroma",
						 "white balance",
						 "filter"
					  ],
					 "version" : "1.3.0",
					 "node" : {
						 "exampleCompositions" : [ "EnhanceBlue.vuo" ]
					 }
				 });

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"
	\n#include "VuoGlslHsl.glsl"

	varying vec2 fragmentTextureCoordinate;
	uniform sampler2D image;

	uniform float saturation;
	uniform float vibrance;
	uniform float brightness;
	uniform float contrast;
	uniform float gamma;
	uniform float exposure;
	uniform float hueShift;
	uniform float temperature;
	uniform float tint;

	const vec3 warmFilter = vec3(0.93, 0.54, 0.0);

	const mat3 RGBtoYIQ = mat3(0.299, 0.587, 0.114, 0.596, -0.274, -0.322, 0.212, -0.523, 0.311);
	const mat3 YIQtoRGB = mat3(1.0, 0.956, 0.621, 1.0, -0.272, -0.647, 1.0, -1.105, 1.702);

	void main(void)
	{
		// https://stackoverflow.com/questions/944713/help-with-pixel-shader-effect-for-brightness-and-contrast
		vec4 pixelColor = VuoGlsl_sample(image, fragmentTextureCoordinate);

		// compensate for channels that are completely void - this allows a fully exposed pixel
		// to always be white, regardless of starting value. .001 is the minimum resolution that
		// enables this.
		if( exposure > 0.)
			pixelColor.rgb += vec3(.001);

		// Apply Exposure
		pixelColor.rgb = clamp(pixelColor.rgb * pow(2., exposure), 0., 1.);

		vec3 hsl = VuoGlsl_rgbToHsl(pixelColor.rgb);

		// Apply hue shift
		hsl.x = mod(hsl.x + hueShift, 1.);

		// // Apply saturation
		hsl.y *= saturation;

		pixelColor.rgb = VuoGlsl_hslToRgb(hsl);


		// Apply vibrance
		float average = (pixelColor.r + pixelColor.g + pixelColor.b) / 3.0;
		float mx = max(pixelColor.r, max(pixelColor.g, pixelColor.b));
		float amt = (mx - average) * (-vibrance * 3.0);
		pixelColor.rgb = mix(pixelColor.rgb, vec3(mx), amt);


		// Apply temperature and tint
		vec3 yiq = RGBtoYIQ * pixelColor.rgb;
		yiq.b = clamp(yiq.b + tint*.5226*.1, -.5226, .5226);
		vec3 rgb = YIQtoRGB * yiq;
		vec3 processed = vec3(
							  (rgb.r < .5 ? (2. * rgb.r * warmFilter.r) : (1. - 2. * (1. - rgb.r) * (1. - warmFilter.r))),
							  (rgb.g < .5 ? (2. * rgb.g * warmFilter.g) : (1. - 2. * (1. - rgb.g) * (1. - warmFilter.g))),
							  (rgb.b < .5 ? (2. * rgb.b * warmFilter.b) : (1. - 2. * (1. - rgb.b) * (1. - warmFilter.b))));
		pixelColor.rgb = mix(rgb, processed, temperature);


		// Apply contrast.
		pixelColor.rgb = ((pixelColor.rgb - 0.5f) * max(contrast, 0.)) + 0.5f;

		// Apply brightness.
		pixelColor.rgb += brightness;
		pixelColor.rgb = clamp(pixelColor.rgb, 0., 1.);

		// // Apply gamma.
		pixelColor.r = pow(pixelColor.r, gamma);
		pixelColor.g = pow(pixelColor.g, gamma);
		pixelColor.b = pow(pixelColor.b, gamma);

		gl_FragColor = pixelColor;
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
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) vibrance,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-180, "suggestedMax":180, "suggestedStep":1}) hueShift,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) temperature,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) tint,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) contrast,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) brightness,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":-1, "suggestedMax":1, "suggestedStep":0.1}) exposure,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":3, "suggestedStep":0.1}) gamma,
		VuoOutputData(VuoImage) adjustedImage
)
{
	if(!image)
	{
		*adjustedImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "image", image);

	// *scaledValue = (value - start) * scaledRange / range + scaledStart;
	VuoShader_setUniform_VuoReal((*instance)->shader, "saturation", MAX(0,saturation+1)); 	// 0 - 3 values
	VuoShader_setUniform_VuoReal((*instance)->shader, "vibrance", vibrance/2.);
	VuoShader_setUniform_VuoReal((*instance)->shader, "hueShift", hueShift/360.);
	VuoShader_setUniform_VuoReal((*instance)->shader, "temperature", temperature);
	VuoShader_setUniform_VuoReal((*instance)->shader, "tint", tint);
	VuoShader_setUniform_VuoReal((*instance)->shader, "brightness", brightness);	// -1, 1 values
	VuoShader_setUniform_VuoReal((*instance)->shader, "contrast", contrast+1);		// 0, 2 values
	VuoShader_setUniform_VuoReal((*instance)->shader, "gamma", gamma);
	VuoShader_setUniform_VuoReal((*instance)->shader, "exposure", exposure*10);

	// Render.
	*adjustedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
