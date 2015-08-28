/**
 * @file
 * vuo.image.resize node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoSizingMode.h"

VuoModuleMetadata({
					  "title" : "Resize Image",
					  "keywords" : [ "scale", "stretch", "fill", "tile", "shrink", "blow up", "enlarge", "magnify" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "EnlargeMovie.vuo" ]
					  }
				 });

static const char * applyScaleFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec2 scale;
	uniform vec2 offset;

	void main(void)
	{
		gl_FragColor = texture2D(texture, (vec2(fragmentTextureCoordinate.x, fragmentTextureCoordinate.y)-offset) * scale);
	}
);

struct nodeInstanceData
{
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

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoSizingMode, {"default":"fit"}) sizingMode,
		VuoInputData(VuoInteger, {"default":640}) width,
		VuoInputData(VuoInteger, {"default":480}) height,
		VuoOutputData(VuoImage) resizedImage
)
{
	if (!image)
		return;

	float u = width / (float)(image->pixelsWide);
	float v = height / (float)(image->pixelsHigh);

	VuoPoint2d scale = (VuoPoint2d) { 1, 1 };
	VuoPoint2d offset = (VuoPoint2d) { 0, 0 };

	switch(sizingMode)
	{
		case VuoSizingMode_Stretch:
			break;

		case VuoSizingMode_Fit:
			if( u < v && u * image->pixelsHigh < height)
			{
				scale = (VuoPoint2d) { 1, height/(image->pixelsHigh*u) };
				offset = (VuoPoint2d) { 0, ((height-(image->pixelsHigh*u))/2)/height };
			}
			else
			{
				scale = (VuoPoint2d) { width/(image->pixelsWide*v), 1 };
				offset = (VuoPoint2d) { ((width-(image->pixelsWide*v))/2)/width, 0 };
			}
			break;

		case VuoSizingMode_Fill:
			if(u > v)
			{
				scale = (VuoPoint2d) { 1, height/(image->pixelsHigh*u) };
				offset = (VuoPoint2d) { 0, ((height-(image->pixelsHigh*u))/2)/height };
			}
			else
			{
				scale = (VuoPoint2d) { width/(image->pixelsWide*v), 1 };
				offset = (VuoPoint2d) { ((width-(image->pixelsWide*v))/2)/width, 0 };
			}
			break;
	}

	VuoShader frag = VuoShader_make("Resize Image Shader");
	VuoShader_addSource(frag, VuoMesh_IndividualTriangles, NULL, NULL, applyScaleFragmentShader);
	VuoRetain(frag);
	VuoShader_setUniform_VuoImage  (frag, "texture", image);
	VuoShader_setUniform_VuoPoint2d(frag, "scale", scale);
	VuoShader_setUniform_VuoPoint2d(frag, "offset", offset);

	*resizedImage = VuoImageRenderer_draw((*instance)->imageRenderer, frag, width, height, VuoImage_getColorDepth(image));

	VuoRelease(frag);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
