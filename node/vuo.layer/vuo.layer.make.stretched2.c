/**
 * @file
 * vuo.layer.make.stretched2 node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Make Image Layer (Stretched)",
	"keywords" : [ "billboard", "sprite", "stretch", "fill", "shrink", "blow up", "enlarge", "magnify", "render", "convert" ],
	"version" : "2.0.0",
	"node": {
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
	VuoInputData(VuoAnchor, { "default": { "horizontalAlignment":"center", "verticalAlignment":"center" } }) anchor,
	VuoInputData(VuoTransform2d) transform,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
	VuoOutputData(VuoLayer) layer
)
{
	VuoLayer base = VuoLayer_makeWithTransform(NULL, image, transform, opacity);
	*layer = VuoLayer_setAnchor(base, anchor, -1, -1, -1);
	VuoLayer_setId(*layer, *id);
}
