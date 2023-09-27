/**
 * @file
 * vuo.image.blur.zoom node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageBlur.h"

VuoModuleMetadata({
					  "title" : "Blur Image Radially",
					  "keywords" : [
						  "filter",
						  "distort", "obscure", "smudge", "censor", "smooth", "soften", "unfocus", "defocus", "detail",
						  "gaussian", "linear", "ramp", "triangle", "box",
						  "motion", "transverse", "zoom", "circular", "spin", "roll", "burst", "streak",
						  "variable", "depth of field", "DOF",
					  ],
					  "version" : "1.1.2",
					  "dependencies" : [
						  "VuoImageBlur"
					  ],
					  "node": {
						  "exampleCompositions" : [ "SimulateMotionBlur.vuo" ]
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
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":4, "suggestedMin":0, "suggestedMax":50}) radius,
		VuoInputData(VuoDispersion, {"default":"linear"}) dispersion,
		VuoInputData(VuoCurveEasing, {"default":"in+out", "includeValues":["in","out","in+out"]}) direction,
		VuoInputData(VuoBoolean, {"default":false}) expandBounds,
		VuoInputData(VuoReal, {"default":0.7, "suggestedMin":0, "suggestedMax":1}) quality,
		VuoOutputData(VuoImage) blurredImage
)
{
	*blurredImage = VuoImageBlur_blurRadially(*blur, image,  mask, shape, center, radius, quality, dispersion, direction, expandBounds);
}
