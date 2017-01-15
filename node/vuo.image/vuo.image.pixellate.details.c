/**
 * @file
 * vuo.image.pixellate node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Pixellate Image with Details",
					 "keywords" : [ "pixels", "lofi", "simplify", "cube", "square", "filter", "overenlarge", "mosaic", "censor",
						 "glass block",
						 "dot screen", "halftone",
						 "cathode ray tube", "crt",
						 "pixelate" // American spelling
					 ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * pixelFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 pixelSize;
	uniform vec2 center;
	uniform float blurriness;
	uniform float coverage;
	uniform float brightnessFactor;
	uniform int numColors;

	void main(void)
	{
		vec2 pos = fragmentTextureCoordinate.xy;

		vec2 centerOffset = mod(center - pixelSize/2, pixelSize);
		vec2 distanceFromCorner = mod(pos - centerOffset, pixelSize);
		pos = pos - distanceFromCorner + pixelSize/2;

		vec2 filterWidth = fwidth(fragmentTextureCoordinate.xy);

		vec4 pixelColor = texture2D(texture, pos);
		const float gamma = .6;
		vec3 pixelColorGamma = pow(pixelColor.rgb, vec3(gamma));

		float luma = dot(pixelColorGamma, vec3(0.299, 0.587, 0.114));

		float c = mix(1, luma, brightnessFactor);
		c += coverage;
		c = (1-min(1,c))/2;

		vec3 quantizedPixelColor = pixelColorGamma * numColors;
		quantizedPixelColor = ceil(quantizedPixelColor);
		quantizedPixelColor = quantizedPixelColor / numColors;
		quantizedPixelColor = pow(quantizedPixelColor, vec3(1.0/gamma));
		quantizedPixelColor = clamp(quantizedPixelColor, 0,1);

		gl_FragColor = mix(vec4(0), vec4(quantizedPixelColor,pixelColor.a),
				 smoothstep(pixelSize.x*   (c-blurriness) -filterWidth.x, pixelSize.x*   (c+blurriness) +filterWidth.x, distanceFromCorner.x)
			* (1-smoothstep(pixelSize.x*(1-(c+blurriness))-filterWidth.x, pixelSize.x*(1-(c-blurriness))+filterWidth.x, distanceFromCorner.x))
			*    smoothstep(pixelSize.y*   (c-blurriness) -filterWidth.y, pixelSize.y*   (c+blurriness) +filterWidth.y, distanceFromCorner.y)
			* (1-smoothstep(pixelSize.y*(1-(c+blurriness))-filterWidth.y, pixelSize.y*(1-(c-blurriness))+filterWidth.y, distanceFromCorner.y)));
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

	instance->shader = VuoShader_make("Pixellate with Details Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, pixelFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0.001, "suggestedMax":2, "suggestedStep":0.05}) pixelSize,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) sharpness,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) coverage,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) brightnessFactor,
		VuoInputData(VuoInteger, {"default":256, "suggestedMin":2, "suggestedMax":256}) colors,
		VuoOutputData(VuoImage) pixellatedImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoPoint2d pixelSize2d = VuoPoint2d_multiply(VuoPoint2d_make(1., (float)w/h), VuoShader_samplerSizeFromVuoSize(pixelSize));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "pixelSize",  pixelSize2d);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "blurriness", 1-MIN(1,sharpness));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "coverage", coverage-1);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "brightnessFactor", brightnessFactor);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "numColors", colors);

	*pixellatedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
