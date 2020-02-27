/**
 * @file
 * vuo.image.toon node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoImageBlur.h"

VuoModuleMetadata({
					 "title" : "Make Cartoon Image",
					 "keywords" : [
						 "comic", "illustration", "hand-drawn", "sketch", "airbrush", "line art", "stroke", "contour",
						 "filter"
					 ],
					 "version" : "1.1.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

// Code from "Graphics Shaders: Theory and Practice" by M. Bailey and S. Cunningham
static const char * vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	attribute vec3 position;
	attribute vec2 textureCoordinate;

	varying vec2 fragmentTextureCoordinate;

	uniform float texelWidth;
	uniform float texelHeight;

	varying vec2 leftTextureCoordinate;
	varying vec2 rightTextureCoordinate;

	varying vec2 topTextureCoordinate;
	varying vec2 topLeftTextureCoordinate;
	varying vec2 topRightTextureCoordinate;

	varying vec2 bottomTextureCoordinate;
	varying vec2 bottomLeftTextureCoordinate;
	varying vec2 bottomRightTextureCoordinate;

	void main()
	{
		gl_Position = vec4(position, 1.);
		fragmentTextureCoordinate = textureCoordinate;

		vec2 widthStep = vec2(texelWidth, 0.0);
		vec2 heightStep = vec2(0.0, texelHeight);
		vec2 widthHeightStep = vec2(texelWidth, texelHeight);
		vec2 widthNegativeHeightStep = vec2(texelWidth, -texelHeight);

		leftTextureCoordinate = textureCoordinate.xy - widthStep;
		rightTextureCoordinate = textureCoordinate.xy + widthStep;

		topTextureCoordinate = textureCoordinate.xy - heightStep;
		topLeftTextureCoordinate = textureCoordinate.xy - widthHeightStep;
		topRightTextureCoordinate = textureCoordinate.xy + widthNegativeHeightStep;

		bottomTextureCoordinate = textureCoordinate.xy + heightStep;
		bottomLeftTextureCoordinate = textureCoordinate.xy - widthNegativeHeightStep;
		bottomRightTextureCoordinate = textureCoordinate.xy + widthHeightStep;
	}
);

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;
	varying vec2 leftTextureCoordinate;
	varying vec2 rightTextureCoordinate;

	varying vec2 topTextureCoordinate;
	varying vec2 topLeftTextureCoordinate;
	varying vec2 topRightTextureCoordinate;

	varying vec2 bottomTextureCoordinate;
	varying vec2 bottomLeftTextureCoordinate;
	varying vec2 bottomRightTextureCoordinate;

	uniform sampler2D texture;

	uniform vec4 edgeColor;
	uniform float threshold;
	uniform float quantizationLevels;
	uniform bool showImage;

	void main()
	{
		vec4 textureColor = texture2D(texture, fragmentTextureCoordinate);

		float bottomLeftIntensity = texture2D(texture, bottomLeftTextureCoordinate).r;
		float topRightIntensity = texture2D(texture, topRightTextureCoordinate).r;
		float topLeftIntensity = texture2D(texture, topLeftTextureCoordinate).r;
		float bottomRightIntensity = texture2D(texture, bottomRightTextureCoordinate).r;
		float leftIntensity = texture2D(texture, leftTextureCoordinate).r;
		float rightIntensity = texture2D(texture, rightTextureCoordinate).r;
		float bottomIntensity = texture2D(texture, bottomTextureCoordinate).r;
		float topIntensity = texture2D(texture, topTextureCoordinate).r;
		float h = -topLeftIntensity - 2.0 * topIntensity - topRightIntensity + bottomLeftIntensity + 2.0 * bottomIntensity + bottomRightIntensity;
		float v = -bottomLeftIntensity - 2.0 * leftIntensity - topLeftIntensity + bottomRightIntensity + 2.0 * rightIntensity + topRightIntensity;

		float mag = length(vec2(h, v));

		vec4 posterizedImageColor = vec4(floor((textureColor.rgb * quantizationLevels) + 0.5) / quantizationLevels, textureColor.a);

		float f = fwidth(mag) / 2.;
		float thresholdTest = smoothstep(threshold-f, threshold+f, mag);

		vec4 c = showImage ? posterizedImageColor : vec4(0.);
		gl_FragColor = mix(c, edgeColor, thresholdTest);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoImageBlur blur;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Make Cartoon Image");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	instance->blur = VuoImageBlur_make();
	VuoRetain(instance->blur);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":4.0, "suggestedMin":0, "suggestedMax":50}) blurRadius,
		VuoInputData(VuoColor, {"default":{"r":0,"g":0,"b":0,"a":1}}) edgeColor,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":10,"suggestedStep":0.1}) edgeWidth,
		VuoInputData(VuoReal, {"default":0.2,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.01}) edgeThreshold,
		VuoInputData(VuoInteger, {"default":6, "suggestedMin":2, "suggestedMax":32}) imageColors,
		VuoInputData(VuoBoolean, {"default":true}) showImage,
		VuoOutputData(VuoImage) cartoonImage
)
{
	if (!image)
	{
		*cartoonImage = NULL;
		return;
	}

	VuoImage blurredImage = VuoImageBlur_blur((*instance)->blur, image, NULL, VuoBlurShape_Linear, blurRadius, 1, false);

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture", blurredImage);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "edgeColor", edgeColor);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "texelWidth", image->scaleFactor * edgeWidth / w);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "texelHeight", image->scaleFactor * edgeWidth / h);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "threshold", edgeThreshold);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "quantizationLevels", imageColors);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "showImage", showImage);

	*cartoonImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->blur);
	VuoRelease((*instance)->shader);
}
