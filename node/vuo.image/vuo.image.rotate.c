/**
 * @file
 * vuo.image.rotate node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRotate.h"

VuoModuleMetadata({
					 "title" : "Rotate Image",
					 "keywords" : [ "spin", "90", "180", "flip" ],
					 "version" : "1.0.2",
					 "dependencies" : [
						 "VuoImageRotate",
					 ],
					 "node": {
						 "exampleCompositions" : [ "RotateImage.vuo" ]
					 }
				 });

VuoImageRotate nodeInstanceInit(void)
{
	return VuoImageRotate_make();
}

void nodeInstanceEvent(
	VuoInstanceData(VuoImageRotate) rotator,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoReal, {"suggestedMin":-180.0, "suggestedMax":180.0, "suggestedStep":15.0}) rotation,
	VuoInputData(VuoBoolean, { "default":false } ) expandBounds,
	VuoOutputData(VuoImage) rotatedImage)
{
	*rotatedImage = VuoImageRotate_rotate(image, *rotator, rotation, expandBounds);
}
