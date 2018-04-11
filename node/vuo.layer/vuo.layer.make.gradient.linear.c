/**
 * @file
 * vuo.layer.make.gradient.linear node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Linear Gradient Layer",
					 "keywords" : [ "backdrop", "background", "billboard", "sprite", "image", "rectangle", "square" ],
					 "version" : "1.2.1",
					 "node": {
						  "exampleCompositions" : [ "CompareLayerGradients.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
		VuoInputData(VuoPoint2d, {"default":{"x":-1, "y":1}, "suggestedMin":{"x":-1, "y":-1}, "suggestedMax":{"x":1, "y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) gradientStart,
		VuoInputData(VuoPoint2d, {"default":{"x":1, "y":-1}, "suggestedMin":{"x":-1, "y":-1}, "suggestedMax":{"x":1, "y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) gradientEnd,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) gradientNoiseAmount,
		VuoInputData(VuoAnchor, { "default": { "horizontalAlignment":"center", "verticalAlignment":"center" } }) layerAnchor,
		VuoInputData(VuoPoint2d, {"name":"Layer Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) layerCenter,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":360.0, "suggestedStep":15.0}) layerRotation,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) layerWidth,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) layerHeight,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_setAnchor(
		VuoLayer_makeLinearGradient(name,
									colors,
									gradientStart,
									gradientEnd,
									layerCenter,
									layerRotation,
									layerWidth,
									layerHeight,
									gradientNoiseAmount),
		layerAnchor,
		-1,
		-1,
		-1);
}
