/**
 * @file
 * vuo.image.mirror node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoHorizontalReflection.h"
#include "VuoVerticalReflection.h"

VuoModuleMetadata({
					  "title" : "Mirror Image",
					  "keywords" : [ "reflect", "flip", "rotate", "horizontal", "vertical", "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "MirrorMovie.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform bool reflectHorizontally;
	uniform bool showLeftHalf;
	uniform bool reflectVertically;
	uniform bool showTopHalf;

	void main(void)
	{
		vec2 mirroredCoordinate = fragmentTextureCoordinate.xy;

		if (reflectHorizontally)
		{
			if ((showLeftHalf && mirroredCoordinate.x > 0.5)
			|| (!showLeftHalf && mirroredCoordinate.x < 0.5))
					mirroredCoordinate.x = 1. - mirroredCoordinate.x;
		}

		if (reflectVertically)
		{
			if ((showTopHalf && mirroredCoordinate.y < 0.5)
			|| (!showTopHalf && mirroredCoordinate.y > 0.5))
				mirroredCoordinate.y = 1. - mirroredCoordinate.y;
		}

		gl_FragColor = texture2D(texture, mirroredCoordinate);
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

	instance->shader = VuoShader_make("Mirror Image Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoHorizontalReflection, {"default":"right"}) horizontalReflection,
		VuoInputData(VuoVerticalReflection, {"default":"none"}) verticalReflection,
		VuoOutputData(VuoImage) reflectedImage
)
{
	if (!image)
		return;

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture",             image);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "reflectHorizontally", horizontalReflection != VuoHorizontalReflection_None);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "showLeftHalf",        horizontalReflection == VuoHorizontalReflection_Left);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "reflectVertically",   verticalReflection != VuoVerticalReflection_None);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "showTopHalf",         verticalReflection == VuoVerticalReflection_Top);

	*reflectedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
