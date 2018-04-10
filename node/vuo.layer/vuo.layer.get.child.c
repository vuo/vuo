/**
 * @file
 * vuo.layer.get.child node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					  "title" : "Get Child Layers",
					  "keywords" : [ ],
					  "version" : "1.0.0",
				  });

void nodeEvent
(
		VuoInputData(VuoLayer) layer,
		VuoOutputData(VuoList_VuoLayer) childLayers
)
{
	*childLayers = VuoLayer_getChildLayers(layer);
}
