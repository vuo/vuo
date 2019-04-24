/**
 * @file
 * vuo.layer.make.oval node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Oval Layer",
					 "keywords" : [ "circle", "rounded", "ellipse", "shape" ],
					 "version" : "1.1.0",
					 "node": {
						  "exampleCompositions" : [ "DrawShapes.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoInputData(VuoAnchor, { "default": { "horizontalAlignment":"center", "verticalAlignment":"center" } }) anchor,
		VuoInputData(VuoPoint2d, {"name":"Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":360.0, "suggestedStep":15.0}) rotation,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) height,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_setAnchor( VuoLayer_makeOval(name, color, center, rotation, width, height, sharpness), anchor, -1, -1, -1);
}
