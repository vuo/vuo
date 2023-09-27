/**
 * @file
 * vuo.layer.make node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Image Layer with Shadow (Scaled)",
					 "keywords" : [ "billboard", "sprite", "image",
						 "stretch", "fill", "shrink", "blow up", "enlarge", "magnify",
						 "glow", "render", "convert"
					 ],
					 "version" : "2.1.1",
					 "node": {
						 "exampleCompositions" : [ "ShowTextShadow.vuo" ]
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
		VuoInputData(VuoImage) image,
		VuoInputData(VuoAnchor, { "default": { "horizontalAlignment":"center", "verticalAlignment":"center" } }) anchor,
		VuoInputData(VuoPoint2d, {"name":"Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-180, "suggestedMax":180, "suggestedStep":15.0}) rotation,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
		VuoInputData(VuoColor, {"default":{"r":0,"g":0,"b":0,"a":1}}) shadowColor,
		VuoInputData(VuoReal, {"default":4.0, "suggestedMin":0, "suggestedMax":20}) shadowBlur,
		VuoInputData(VuoReal, {"default":315.0, "suggestedMin":0, "suggestedMax":360}) shadowAngle,
		VuoInputData(VuoReal, {"default":0.005, "suggestedMin":0, "suggestedMax":0.1, "suggestedStep":0.005}) shadowDistance,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_setAnchor(
		VuoLayer_makeWithShadow(name,
								image,
								center,
								rotation,
								width,
								opacity,
								shadowColor,
								shadowBlur,
								shadowAngle,
								shadowDistance),
		anchor,
		-1,
		-1,
		-1);
	VuoLayer_setId(*layer, *id);
}
