/**
 * @file
 * vuo.image.dilate node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoImageDilate.h"

VuoModuleMetadata({
	"title" : "Dilate Image",
	"keywords" : [
		"bolden", "lighten", "grow", "expand", "enlarge", "maximum", "patch holes", "fill gaps", "open",
		"erode", "darken", "shrink", "contract", "minimum", "close", "reduce",
		"noise reduction", "simplify", "smooth",
		"morphological", "morphology",
		"pointillism", "impressionism",
		"mask",
		"filter"
	],
	"version" : "1.1.0",
	"dependencies" : [
		"VuoImageDilate"
	],
	"node": {
		"exampleCompositions" : [ "DilateImage.vuo" ]
	}
});

VuoImageDilate *nodeInstanceInit(void)
{
	return VuoImageDilate_make();
}

void nodeInstanceEvent
(
	VuoInstanceData(VuoImageDilate *) dilate,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoBlurShape, {"default":"disc", "includeValues":["box","disc"]}) shape,
	VuoInputData(VuoReal, {"default":4.0,"suggestedMin":-10.0,"suggestedMax":10.0,"suggestedStep":1.0}) radius,
	VuoInputData(VuoBoolean, {"default":false}) rescind,
	VuoOutputData(VuoImage) dilatedImage
)
{
	*dilatedImage = VuoImageDilate_dilate(*dilate, image, shape, radius, rescind);
}
