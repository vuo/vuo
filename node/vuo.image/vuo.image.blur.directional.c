/**
 * @file
 * vuo.image.blur.directional node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageBlur.h"

VuoModuleMetadata({
					  "title" : "Blur Image Directionally",
					  "keywords" : [
						  "filter",
						  "distort", "obscure", "smudge", "censor", "smooth", "soften", "unfocus", "defocus", "detail",
						  "gaussian", "linear", "ramp", "triangle", "box",
						  "motion", "linear", "horizontal", "vertical", "diagonal",
						  "variable", "depth of field", "DOF",
					  ],
					  "version" : "1.1.2",
					  "dependencies" : [
						  "VuoImageBlur"
					  ],
					  "node": {
						  "exampleCompositions" : [ "CompareDirectionalBlurs.vuo" ]
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
		VuoInputData(VuoBlurShape, {"default":"linear", "includeValues":["gaussian", "linear", "box"]}) shape, // This is a 1D blur, so the "disc" shape doesn't make sense.
		VuoInputData(VuoReal, {"default":4, "suggestedMin":0, "suggestedMax":50}) radius,
		VuoInputData(VuoReal, {"default":0., "suggestedMin":0., "suggestedMax":360.}) angle,
		VuoInputData(VuoBoolean, {"default":true}) symmetric,
		VuoInputData(VuoBoolean, {"default":false}) expandBounds,
		VuoInputData(VuoReal, {"default":0.7, "suggestedMin":0, "suggestedMax":1}) quality,
		VuoOutputData(VuoImage) blurredImage
)
{
	*blurredImage = VuoImageBlur_blurDirectionally(*blur, image, mask, shape, radius, quality, angle, symmetric, expandBounds);
}
