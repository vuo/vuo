/**
 * @file
 * vuo.list.get.range node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoList_VuoIntegerRange.h"

VuoModuleMetadata({
					 "title" : "Get Item Ranges from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index", "indices" ],
					 "version" : "1.0.2",
					 "node": {
						  "exampleCompositions" : [ "ReorderRangesOfItems.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoList_VuoIntegerRange) ranges,
		VuoOutputData(VuoList_VuoGenericType1) items
)
{
	VuoInteger rangeCount = VuoListGetCount_VuoIntegerRange(ranges);
	VuoInteger listCount = VuoListGetCount_VuoGenericType1(list);
	if (rangeCount == 0 || listCount == 0)
	{
		*items = NULL;
		return;
	}

	VuoIntegerRange *rangeData = VuoListGetData_VuoIntegerRange(ranges);
	unsigned long outputItemCount = 0;
	for (unsigned long i = 0; i < rangeCount; ++i)
	{
		VuoIntegerRange range = VuoIntegerRange_getOrderedRange(rangeData[i]);
		for (VuoInteger n = MAX(range.minimum, 1); n <= MIN(range.maximum, listCount); n++)
			++outputItemCount;
	}
	if (outputItemCount == 0)
	{
		*items = NULL;
		return;
	}

	*items = VuoListCreateWithCount_VuoGenericType1(outputItemCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *itemsData = VuoListGetData_VuoGenericType1(*items);

	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);
	unsigned long m = 0;
	for (unsigned long i = 0; i < rangeCount; ++i)
	{
		VuoIntegerRange range = VuoIntegerRange_getOrderedRange(rangeData[i]);
		for (VuoInteger n = MAX(range.minimum, 1); n <= MIN(range.maximum, listCount); n++)
		{
			itemsData[m] = listData[VuoListIndexToCArrayIndex(n, listCount)];
			VuoGenericType1_retain(itemsData[m++]);
		}
	}
}
