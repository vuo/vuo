/**
 * @file
 * vuo.image.blur node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageBlur.h"

VuoModuleMetadata({
					  "title" : "Blur Image",
					  "keywords" : [ "gaussian", "distort", "obscure", "smudge", "filter", "censor", "smooth", "soften", "unfocus", "defocus", "detail" ],
					  "version" : "1.2.1",
					  "dependencies" : [
						  "VuoImageBlur"
					  ],
					  "node": {
						  "exampleCompositions" : [ "BlurMovie.vuo" ]
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
		VuoInputData(VuoReal, {"default":4, "suggestedMin":0, "suggestedMax":20}) radius,
		VuoInputData(VuoBoolean, {"default":false}) expandBounds,
		VuoOutputData(VuoImage) blurredImage
)
{
	*blurredImage = VuoImageBlur_blur(*blur, image, radius, expandBounds);
}

void nodeInstanceFini(VuoInstanceData(VuoImageBlur *) blur)
{
}
