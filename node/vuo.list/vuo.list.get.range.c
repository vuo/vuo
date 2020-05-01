/**
 * @file
 * vuo.list.get.range node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Item Ranges from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index", "indices" ],
					 "version" : "1.0.1",
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
	if (VuoListGetCount_VuoIntegerRange(ranges) == 0
	 || VuoListGetCount_VuoGenericType1(list) == 0)
	{
		*items = NULL;
		return;
	}

	*items = VuoListCreate_VuoGenericType1();

	VuoInteger listCount = VuoListGetCount_VuoGenericType1(list);
	VuoInteger rangeCount = VuoListGetCount_VuoIntegerRange(ranges);

	for(unsigned long i = 1; i <= rangeCount; ++i)
	{
		VuoIntegerRange range = VuoIntegerRange_getOrderedRange(VuoListGetValue_VuoIntegerRange(ranges, i));

		for(VuoInteger n = MAX(range.minimum, 1); n <= MIN(range.maximum, listCount); n++)
			VuoListAppendValue_VuoGenericType1(*items, VuoListGetValue_VuoGenericType1(list, n));
	}
}
