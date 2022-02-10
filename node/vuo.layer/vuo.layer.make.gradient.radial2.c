/**
 * @file
 * vuo.layer.make.gradient.radial2 node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Make Radial Gradient Layer",
	"keywords" : [ "backdrop", "background", "billboard", "sprite", "image",
		"circle", "oval", "ellipse", "rounded", "shape" ],
	"version" : "2.0.0",
	"node": {
		"exampleCompositions" : [ "CompareLayerGradients.vuo" ]
	}
});

uint64_t nodeInstanceInit(void)
{
	return VuoSceneObject_getNextId();
}

void nodeInstanceEvent
(
	VuoInstanceData(uint64_t) id,
	VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) gradientCenter,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) gradientRadius,
	VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) gradientNoiseAmount,
	VuoInputData(VuoAnchor, { "default": { "horizontalAlignment":"center", "verticalAlignment":"center" } }) layerAnchor,
	VuoInputData(VuoPoint2d, {"name":"Layer Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) layerCenter,
	VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-180, "suggestedMax":180, "suggestedStep":15.0}) layerRotation,
	VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) layerWidth,
	VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) layerHeight,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) layerOpacity,
	VuoOutputData(VuoLayer) layer
)
{
	VuoList_VuoColor c = VuoListCopy_VuoColor(colors);
	VuoLocal(c);
	unsigned long count = VuoListGetCount_VuoColor(c);
	VuoColor *data = VuoListGetData_VuoColor(c);
	for (unsigned long i = 0; i < count; ++i)
		data[i].a *= layerOpacity;

	*layer = VuoLayer_setAnchor(
		VuoLayer_makeRadialGradient(NULL,
									c,
									gradientCenter,
									gradientRadius,
									layerCenter,
									layerRotation,
									layerWidth,
									layerHeight,
									gradientNoiseAmount),
		layerAnchor,
		-1,
		-1,
		-1);
	VuoLayer_setId(*layer, *id);
}
