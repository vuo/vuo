/**
 * @file
 * vuo.image.resize node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoSizingMode.h"
#include "VuoImageResize.h"

VuoModuleMetadata({
					  "title" : "Resize Image",
					  "keywords" : [ "size", "scale", "stretch", "fill", "tile", "shrink", "blow up", "enlarge", "magnify" ],
					  "version" : "1.1.3",
					  "node": {
						  "exampleCompositions" : [ "EnlargeImage.vuo" ]
					  },
					  "dependencies" : [
						"VuoImageResize"
					 ]
				 });

struct nodeInstanceData
{
	VuoImageResize resize;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

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
		*resizedImage = VuoImageResize_resize(image, (*instance)->resize, sizingMode, width, height);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->resize);
}
