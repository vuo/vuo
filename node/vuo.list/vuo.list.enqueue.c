/**
 * @file
 * vuo.list.enqueue node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Enqueue",
					  "keywords" : [ "history", "collect", "hoard", "populate", "slot", "dispense", "append", "accumulate",
						  "limit", "capacity", "queue", "fifo", "first in first out", "recent", "hold", "store" ],
					  "version" : "1.1.0",
					  "node": {
						  "exampleCompositions": [ ]
					  }
				 });


VuoList_VuoGenericType1 nodeInstanceInit
(
		VuoInputData(VuoInteger) maxItemCount
)
{
	return VuoListCreate_VuoGenericType1();
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoList_VuoGenericType1) listInstanceData,
		VuoInputData(VuoInteger, { "default":10, "suggestedMin":0, "auto":infinity }) maxItemCount,
		VuoInputEvent({"eventBlocking":"wall","data":"maxItemCount"}) maxItemCountEvent,
		VuoInputData(VuoGenericType1) addItem,
		VuoInputEvent({"eventBlocking":"none","data":"addItem"}) addItemEvent,
		VuoInputEvent({"eventBlocking":"none"}) clearList,
		VuoOutputData(VuoList_VuoGenericType1) list
)
{
	if (clearList)
		VuoListRemoveAll_VuoGenericType1(*listInstanceData);

	if (addItemEvent)
		VuoListAppendValue_VuoGenericType1(*listInstanceData, addItem);

	unsigned long clampedMaxItemCount = MAX(0, maxItemCount);
	unsigned long itemCount = VuoListGetCount_VuoGenericType1(*listInstanceData);
	for ( ; itemCount > clampedMaxItemCount; --itemCount)
		VuoListRemoveFirstValue_VuoGenericType1(*listInstanceData);

	*list = VuoListCreate_VuoGenericType1();
	VuoGenericType1* listCopy = VuoListGetData_VuoGenericType1(*listInstanceData);
	for (unsigned long i = 1; i <= itemCount; ++i)
		VuoListAppendValue_VuoGenericType1( *list, listCopy[i-1] );
}

void nodeInstanceFini
(
		VuoInstanceData(VuoList_VuoGenericType1) listInstanceData
)
{
}
