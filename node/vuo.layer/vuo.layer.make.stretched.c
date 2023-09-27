/**
 * @file
 * vuo.layer.make.stretched node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Image Layer (Stretched)",
					 "keywords" : [ "billboard", "sprite", "stretch", "fill", "shrink", "blow up", "enlarge", "magnify", "render", "convert" ],
					 "version" : "1.1.0",
					 "node": {
						 "isDeprecated": true
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
		VuoInputData(VuoTransform2d) transform,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
		VuoOutputData(VuoLayer) layer
)
{
	VuoLayer base = VuoLayer_makeWithTransform(name, image, transform, opacity);
	*layer = VuoLayer_setAnchor(base, anchor, -1, -1, -1);
	VuoLayer_setId(*layer, *id);
}
