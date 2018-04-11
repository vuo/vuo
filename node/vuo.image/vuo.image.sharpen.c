/**
 * @file
 * vuo.image.sharpen node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageBlur.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Sharpen Image",
					  "keywords" : [
						  "filter",
						  "unsharp mask", "USM", "high-pass filter", "edge", "detail", "enhance",
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoImageBlur",
						  "VuoImageRenderer",
					  ],
					  "node": {
						  "exampleCompositions" : [ "SharpenMovie.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D image;
	uniform sampler2D blurredImage;
	uniform float amount;
	uniform float threshold;

	void main(void)
	{
		vec4 imageColor        = VuoGlsl_sample(image,        fragmentTextureCoordinate.xy);
		vec4 blurredImageColor = VuoGlsl_sample(blurredImage, fragmentTextureCoordinate.xy);

		vec4 difference = vec4(imageColor.rgb - blurredImageColor.rgb, 0.);
		float dist = length(difference);
		float f = fwidth(dist);
		vec4 c = imageColor + smoothstep(threshold-f, threshold+f, dist) * difference * amount;
		c.rgb = clamp(c.rgb, 0., c.a);
		gl_FragColor = c;
	}
);

struct nodeInstanceData
{
	VuoImageBlur blur;

	VuoGlContext glContext;
	VuoShader shader;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->blur = VuoImageBlur_make();
	VuoRetain(instance->blur);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Sharpen Image Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoReal, {"default":4.0, "suggestedMin":0.0, "suggestedMax":50.0}) radius,
	VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":5.0}) amount,
	VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":1.0}) threshold,
	VuoOutputData(VuoImage) sharpenedImage
)
{
	if (!image || VuoReal_areEqual(radius, 0))
	{
		*sharpenedImage = image;
		return;
	}

	VuoImage blurredImage = VuoImageBlur_blur((*instance)->blur, image, NULL, VuoBlurShape_Gaussian, radius, 1, false);
	VuoLocal(blurredImage);

	VuoShader_setUniform_VuoImage((*instance)->shader, "image",        image);
	VuoShader_setUniform_VuoImage((*instance)->shader, "blurredImage", blurredImage);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "amount",       amount);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "threshold",    threshold);
	*sharpenedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(blurredImage));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->blur);
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
