/**
 * @file
 * vuo.image.blur node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageBlur.h"

VuoModuleMetadata({
					  "title" : "Blur Image",
					  "keywords" : [
						  "filter",
						  "distort", "obscure", "smudge", "censor", "smooth", "soften", "unfocus", "defocus", "detail",
						  "gaussian", "linear", "ramp", "triangle", "box", "disc", "disk",
						  "variable", "depth of field", "DOF",
					  ],
					  "version" : "1.4.2",
					  "dependencies" : [
						  "VuoImageBlur"
					  ],
					  "node": {
						  "exampleCompositions" : [ "BlurImage.vuo" ]
					  }
				 });

VuoImageBlur *nodeInstanceInit(void)
{
	return VuoImageBlur_make();
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoImageBlur *) blur,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoImage) mask,
		VuoInputData(VuoBlurShape, {"default":"linear"}) shape,
		VuoInputData(VuoReal, {"default":4, "suggestedMin":0, "suggestedMax":50}) radius,
		VuoInputData(VuoBoolean, {"default":false}) expandBounds,
		VuoInputData(VuoReal, {"default":0.7, "suggestedMin":0, "suggestedMax":1}) quality,
		VuoOutputData(VuoImage) blurredImage
)
{
	*blurredImage = VuoImageBlur_blur(*blur, image, mask, shape, radius, quality, expandBounds);
}
