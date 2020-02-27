/**
 * @file
 * vuo.layer.get node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Get Layer Values",
					 "keywords" : [ "information", "children", "transform" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoLayer) layer,
		VuoOutputData(VuoTransform2d) transform,
		VuoOutputData(VuoList_VuoLayer) childLayers
)
{
	*transform = VuoTransform_get2d(VuoSceneObject_getTransform((VuoSceneObject)layer));
	*childLayers = VuoLayer_getChildLayers(layer);
}
