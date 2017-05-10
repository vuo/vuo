/**
 * @file
 * vuo.image.resize.larger node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoSizingMode.h"
#include "VuoImageResize.h"

VuoModuleMetadata({
					 "title" : "Resize Image if Larger",
					 "keywords" : [ "size", "scale", "stretch", "fill", "tile", "shrink", "blow up", "enlarge", "magnify" ],
					 "version" : "1.1.1",
					 "node" : {
						 "exampleCompositions" : [ ]
					 },
					 "dependencies": [
					 	"VuoImageResize"
					 ]
				 });

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

	instance->shader = VuoImageResize_makeShader();
	VuoRetain(instance->shader);

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

	if(image->pixelsWide < width && image->pixelsHigh < height)
		*resizedImage = image;
	else
		*resizedImage = VuoImageResize_resize(image, (*instance)->shader, (*instance)->imageRenderer, sizingMode, width, height);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
