/**
 * @file
 * vuo.data.hold.list2 node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Hold List",
					 "keywords" : [ "store", "retain", "keep", "sample", "preserve", "feedback", "loop", "control flow", "block", "wall" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "StoreMousePosition.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputEvent() update,
	VuoInputData(VuoList_VuoGenericType1) value,
	VuoInputEvent({"eventBlocking":"wall","data":"value"}) valueEvent,
	VuoOutputData(VuoList_VuoGenericType1) heldValue
)
{
	if (update)
		*heldValue = value;
}
