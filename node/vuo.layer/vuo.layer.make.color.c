/**
 * @file
 * vuo.layer.make.color node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Color Layer",
					 "keywords" : [ "backdrop", "background", "billboard", "sprite", "image", "tone", "chroma",
									"rectangle", "square", "shape" ],
					 "version" : "1.2.0",
					 "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "DrawShapes.vuo" ]
					 }
				 });

uint64_t nodeInstanceInit(void)
{
	return VuoSceneObject_getNextId();
}

void nodeInstanceEvent
(
		VuoInstanceData(uint64_t) id,
		VuoInputData(VuoText) name,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoInputData(VuoAnchor) anchor,
		VuoInputData(VuoPoint2d, {"name":"Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-180, "suggestedMax":180, "suggestedStep":15.0}) rotation,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) height,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_setAnchor(VuoLayer_makeColor(name, color, center, rotation, width, height), anchor, -1, -1, -1);
	VuoLayer_setId(*layer, *id);
}
