/**
 * @file
 * vuo.image.crop node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Crop Image",
					  "keywords" : [ "resize", "snip", "clip", "sample", "rectangle", "trim" ],
					  "version" : "1.1.2",
					  "node": {
						  "exampleCompositions" : [ "EnlargeMovie.vuo" ]
					  }
				 });

static const char * cropFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec2 offset;
	uniform float width;
	uniform float height;

	void main(void)
	{
		gl_FragColor = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy * vec2(width, height) + offset);
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

	instance->shader = VuoShader_make("Crop Image Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, cropFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) height,
		VuoOutputData(VuoImage) croppedImage
)
{
	if (!image)
	{
		*croppedImage = NULL;
		return;
	}

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

	*croppedImage = VuoImageRenderer_render((*instance)->shader, outputWidth, outputHeight, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
