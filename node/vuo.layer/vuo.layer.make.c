/**
 * @file
 * vuo.layer.make node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Layer",
					 "keywords" : [ "billboard", "sprite", "image" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [
							 "DisplayImagesOnLayers.vuo",
							 "RotateGears.vuo"
						 ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":360.0, "suggestedStep":1.0}) rotation,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) alpha,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_make(image, center, rotation, width, alpha);
}
