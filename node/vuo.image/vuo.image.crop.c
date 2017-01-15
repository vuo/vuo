/**
 * @file
 * vuo.image.crop node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Crop Image",
					  "keywords" : [ "resize", "snip", "clip", "sample", "rectangle", "trim" ],
					  "version" : "1.1.1",
					  "node": {
						  "exampleCompositions" : [ "EnlargeMovie.vuo" ]
					  }
				 });

static const char * cropFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec2 offset;
	uniform float width;
	uniform float height;

	void main(void)
	{
		vec2 pos = fragmentTextureCoordinate.xy * vec2(width, height) + offset;
		gl_FragColor = texture2D(texture, pos);
	}
);

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoShader shader;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Crop Image Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, cropFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) height,
		VuoOutputData(VuoImage) croppedImage
)
{
	if (!image)
		return;

	VuoRectangle intersection = VuoPoint2d_rectangleIntersection(
				VuoImage_getRectangle(image),
				VuoRectangle_make(center.x,center.y,width,height)
				);

	VuoPoint2d samplerCenter = VuoShader_samplerCoordinatesFromVuoCoordinates(intersection.center, image);
	float samplerWidth = VuoShader_samplerSizeFromVuoSize(intersection.size.x);
	float samplerHeight = VuoShader_samplerSizeFromVuoSize(intersection.size.y) * image->pixelsWide/image->pixelsHigh;

	int outputWidth  = samplerWidth * image->pixelsWide;
	int outputHeight = samplerHeight * image->pixelsHigh;

	if (outputWidth < 1 || outputHeight < 1)
	{
		*croppedImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "offset",  VuoPoint2d_subtract(samplerCenter, VuoPoint2d_make(samplerWidth/2., samplerHeight/2.)));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "width",   samplerWidth);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "height",  samplerHeight);

	*croppedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, outputWidth, outputHeight, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
