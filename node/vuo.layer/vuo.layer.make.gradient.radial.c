/**
 * @file
 * vuo.layer.make.gradient.radial node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Radial Gradient Layer",
					 "keywords" : [ "backdrop", "background", "billboard", "sprite", "image",
									"circle", "oval", "ellipse", "rounded" ],
					 "version" : "1.1.0",
					 "node": {
						  "exampleCompositions" : [ "CompareLayerGradients.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
		VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}, "suggestedStep":{"x":0.1,"y":0.1}}) gradientCenter,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0.0, "suggestedStep":0.1}) gradientRadius,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) gradientNoiseAmount,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedStep":{"x":0.1,"y":0.1}}) layerCenter,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":360.0, "suggestedStep":15.0}) layerRotation,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) layerWidth,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) layerHeight,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_makeRadialGradient(name, colors, gradientCenter, gradientRadius, layerCenter, layerRotation, layerWidth, layerHeight, gradientNoiseAmount);
}
