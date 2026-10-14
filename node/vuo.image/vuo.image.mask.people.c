/**
 * @file
 * vuo.image.mask.people node implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPeopleMask.h"

VuoModuleMetadata({
	"title" : "Extract People from Image",
	"keywords" : [
		"search", "segmentation", "detection", "tracking", "features", "landmarks", "locate",
		"computer vision", "cv",
		"artificial intelligence", "ai",
		"machine learning", "ml",
		"humans", "person", "faces", "heads", "portraits", "body", "bodies",
		"foreground", "background",
		"removal", "subtraction", "exclusion", "separation", "extraction", "lift",
		"chromakey", "matte", "alpha", "opacity", "transparency", "outline", "superimpose",
	],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoPeopleMask",
	],
	"node" : {
		"exampleCompositions" : [ "ShowExtractedPeople.vuo", "InstantEntourage.vuo" ]
	},
	"compatibility": {
		"macos": {
			"min": "12.0",
		},
	},
});

VuoPeopleMask nodeInstanceInit(void)
{
	return VuoPeopleMask_make();
}

void nodeInstanceEvent(
	VuoInstanceData(VuoPeopleMask) peopleMask,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.5}) quality,
	VuoOutputData(VuoImage) people
)
{
	*people = NULL;
	VuoImage_performWithIOSurface(image, false, ^(IOSurfaceRef ioSurface){
		*people = VuoPeopleMask_processImage(*peopleMask, ioSurface, image, quality);
	});
}
