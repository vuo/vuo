/**
 * @file
 * example.imageFilter.coreImage node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "node.h"

#import "ExampleImageFilterCoreImage.h"

VuoModuleMetadata({
	"title" : "Twirl Image using Core Image",
	"dependencies" : [
		"ExampleImageFilterCoreImage",
		"CoreImage.framework",
	],
});

void *nodeInstanceInit()
{
	__block void *t;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		t = ExampleImageFilterCoreImage_make(cgl_ctx);
	});
	return t;
}

void nodeInstanceEvent(
	VuoInstanceData(void *) t,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoPoint2d, {"default":[150,150]}) position,
	VuoInputData(VuoReal, {"default":300}) radius,
	VuoInputData(VuoReal, {"default":3.14}) angle,
	VuoOutputData(VuoImage) outputImage)
{
	if (!image)
		return;

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		*outputImage = ExampleImageFilterCoreImage_processImage(*t, image, position, radius, angle);
	});
}
