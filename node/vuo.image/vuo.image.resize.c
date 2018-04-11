/**
 * @file
 * vuo.image.resize node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoSizingMode.h"
#include "VuoImageResize.h"

VuoModuleMetadata({
					  "title" : "Resize Image",
					  "keywords" : [ "size", "scale", "stretch", "fill", "tile", "shrink", "blow up", "enlarge", "magnify" ],
					  "version" : "1.1.2",
					  "node": {
						  "exampleCompositions" : [ "EnlargeMovie.vuo" ]
					  },
					  "dependencies" : [
						"VuoImageResize"
					 ]
				 });

struct nodeInstanceData
{
	VuoImageResize resize;
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

	instance->resize = VuoImageResize_make();
	VuoRetain(instance->resize);

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
	if(!image)
	{
		*resizedImage = NULL;
		return;
	}

	if(image->pixelsWide == width && image->pixelsHigh == height)
		*resizedImage = image;
	else
		*resizedImage = VuoImageResize_resize(image, (*instance)->resize, (*instance)->imageRenderer, sizingMode, width, height);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->resize);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
