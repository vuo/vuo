/**
 * @file
 * vuo.layer.combine node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Combine Layers (Transform)",
					 "keywords" : [ "group", "join", "together", "merge" ],
					 "version" : "1.0.0",
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
		VuoInputData(VuoTransform2d) transform,
		VuoInputData(VuoList_VuoLayer) layers,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_makeGroup(layers, transform);
	VuoLayer_setId(*layer, *id);
}
