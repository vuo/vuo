/**
 * @file
 * example.imageFilter.metal node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "ExampleImageFilterMetal.h"

VuoModuleMetadata({
	"title" : "Ripple Image using Metal",
	"dependencies" : [
		"ExampleImageFilterMetal",
		"Metal.framework",
	],
});

void *nodeInstanceInit()
{
	return ExampleImageFilterMetal_make();
}

void nodeInstanceEvent(
	VuoInstanceData(void *) t,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoReal) phase,
	VuoOutputData(VuoImage) outputImage)
{
	if (!image)
	{
		*outputImage = NULL;
		return;
	}

	json_object *imageJson = VuoImage_getInterprocessJson(image);
	json_object *imageId;
	json_object_object_get_ex(imageJson, "ioSurface", &imageId);
	IOSurfaceRef imageSurf = IOSurfaceLookup(json_object_get_int(imageId));

	IOSurfaceRef outputImageSurf = ExampleImageFilterMetal_processImage(*t, imageSurf, phase);

	VuoIoSurfacePool_signal(imageSurf);
	CFRelease(imageSurf);
	json_object_put(imageJson);

	if (!outputImageSurf)
	{
		*outputImage = NULL;
		return;
	}

	json_object *outputImageJson = json_object_new_object();
	json_object_object_add(outputImageJson, "ioSurface", json_object_new_int(IOSurfaceGetID(outputImageSurf)));
	json_object_object_add(outputImageJson, "pixelsWide", json_object_new_int64(IOSurfaceGetWidth(outputImageSurf)));
	json_object_object_add(outputImageJson, "pixelsHigh", json_object_new_int64(IOSurfaceGetHeight(outputImageSurf)));
	*outputImage = VuoImage_makeFromJson(outputImageJson);
	json_object_put(outputImageJson);
	CFRelease(outputImageSurf);
}
