/**
 * @file
 * vuo.list.get.multiple node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Items from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index", "indices", "reorder", "rearrange", "shuffle", "combination" ],
					 "version" : "1.0.2",
					 "node": {
						  "exampleCompositions" : [ "SelectLayerFromList.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoList_VuoInteger) positions,
		VuoOutputData(VuoList_VuoGenericType1) items
)
{
	unsigned long positionCount = VuoListGetCount_VuoInteger(positions);
	if (positionCount == 0)
	{
		*items = NULL;
		return;
	}

	*items = VuoListCreateWithCount_VuoGenericType1(positionCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *itemsData = VuoListGetData_VuoGenericType1(*items);

	unsigned long listCount = VuoListGetCount_VuoGenericType1(list);
	if (listCount == 0)
		return;

	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);

	VuoInteger *positionsData = VuoListGetData_VuoInteger(positions);
	for (int i = 0; i < positionCount; ++i)
	{
		itemsData[i] = listData[VuoListIndexToCArrayIndex(positionsData[i], listCount)];
		VuoGenericType1_retain(itemsData[i]);
	}
}
