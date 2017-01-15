/**
 * @file
 * vuo.image.color.combine.rgb node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Combine Image RGB Channels",
					  "keywords" : [ "sum", "add", "colors", "filter" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "SeparateRedGreenBlue.vuo", "EnhanceBlue.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D redTexture;
	uniform sampler2D greenTexture;
	uniform sampler2D blueTexture;

	void main(void)
	{
		vec4 redColor   = texture2D(redTexture, fragmentTextureCoordinate.xy);
		vec4 greenColor = texture2D(greenTexture, fragmentTextureCoordinate.xy);
		vec4 blueColor  = texture2D(blueTexture, fragmentTextureCoordinate.xy);
		gl_FragColor = vec4(
					(redColor.r   + redColor.g   + redColor.b  )/3.,
					(greenColor.r + greenColor.g + greenColor.b)/3.,
					(blueColor.r  + blueColor.g  + blueColor.b )/3.,
					(redColor.a   + greenColor.a + blueColor.a )/3.);
	}
);

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Combine Image RGB Colors Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) redImage,
		VuoInputData(VuoImage) greenImage,
		VuoInputData(VuoImage) blueImage,
		VuoOutputData(VuoImage) combinedImage
)
{
	VuoImage provokingImage = NULL;
	if (redImage)
		provokingImage = redImage;
	else if (greenImage)
		provokingImage = greenImage;
	else if (blueImage)
		provokingImage = blueImage;
	else
	{
		*combinedImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "redTexture",   redImage);
	VuoShader_setUniform_VuoImage((*instance)->shader, "greenTexture", greenImage);
	VuoShader_setUniform_VuoImage((*instance)->shader, "blueTexture",  blueImage);
	*combinedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, provokingImage->pixelsWide, provokingImage->pixelsHigh, VuoImage_getColorDepth(provokingImage));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
