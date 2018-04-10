/**
 * @file
 * vuo.layer.make node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Image Layer (Scaled)",
					 "keywords" : [ "billboard", "sprite", "scaled",
						 "stretch", "fill", "shrink", "blow up", "enlarge", "magnify" ],
					 "version" : "2.1.0",
					 "node": {
						 "exampleCompositions" : [ "DisplayImagesOnLayers.vuo", "RotateGears.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoAnchor, { "default": { "horizontalAlignment":"center", "verticalAlignment":"center" } }) anchor,
		VuoInputData(VuoPoint2d, {"name":"Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":360.0, "suggestedStep":15.0}) rotation,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
		VuoOutputData(VuoLayer) layer
)
{
	VuoLayer base = VuoLayer_make(name, image, center, rotation, width, opacity);
	*layer = VuoLayer_setAnchor(base, anchor, -1, -1, -1);
}
