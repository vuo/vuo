/**
 * @file
 * vuo.layer.make.rect node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Make Rectangle Layer",
	"keywords" : [
		"capsule", "corners", "square", "button", "shape", "rounded", "border-radius",
		"backdrop", "background", "billboard", "sprite",
	],
	"version" : "1.0.0",
	"node": {
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
	VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) color,
	VuoInputData(VuoAnchor, { "default": { "horizontalAlignment":"center", "verticalAlignment":"center" } }) anchor,
	VuoInputData(VuoPoint2d, {"name":"Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
	VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-180, "suggestedMax":180, "suggestedStep":15.0}) rotation,
	VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) width,
	VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) height,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) sharpness,
	VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) roundness,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
	VuoOutputData(VuoLayer) layer
)
{
	VuoColor c = color;
	c.a *= opacity;
	if (VuoReal_areEqual(sharpness, 1) && VuoReal_areEqual(roundness, 0))
		*layer = VuoLayer_makeColor(NULL, c, center, rotation, width, height);
	else
		*layer = VuoLayer_makeRoundedRectangle(NULL, c, center, rotation, width, height, sharpness, roundness);
	*layer = VuoLayer_setAnchor(*layer, anchor, -1, -1, -1);
	VuoLayer_setId(*layer, *id);
}
