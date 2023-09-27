/**
 * @file
 * vuo.layer.make.realSize2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Make Image Layer (Real Size)",
	"keywords" : [ "billboard", "sprite", "image", "pixel aligned", "exact", "actual", "render", "convert" ],
	"version" : "3.0.0",
	"node" : {
		"exampleCompositions" : [ ]
	}
});

uint64_t nodeInstanceInit(void)
{
	return VuoSceneObject_getNextId();
}

void nodeInstanceEvent
(
	VuoInstanceData(uint64_t) id,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
	VuoInputData(VuoBoolean, {"default":false}) preservePhysicalSize,
	VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_makeRealSize(NULL, image, center, opacity, preservePhysicalSize);
	VuoLayer_setId(*layer, *id);
}
