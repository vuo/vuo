/**
 * @file
 * vuo.list.comb node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Comb List",
					 "keywords" : [
						 "get", "select", "choose",
						 "element", "member", "index",
						 "alternate", "alternating", "every other", "some", "nth"
					 ],
					 "version" : "1.0.1",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1}) pickCount,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":0}) skipCount,
		VuoOutputData(VuoList_VuoGenericType1) pickedItems
)
{
	if (pickCount < 1)
	{
		*pickedItems = NULL;
		return;
	}

	unsigned long count = VuoListGetCount_VuoGenericType1(list);
	if (count == 0)
	{
		*pickedItems = NULL;
		return;
	}

	unsigned long skipCountClamped = MAX(0, skipCount);
	unsigned long picked = 0;
	unsigned long skipped = 0;
	unsigned long pickedItemCount = 0;
	for (unsigned long i = 0; i < count; ++i)
	{
		if (picked < pickCount)
			++picked;
		else
		{
			if (skipped < skipCountClamped)
			{
				++skipped;
				continue;
			}
			else
			{
				skipped = 0;
				picked = 1;
			}
		}

		++pickedItemCount;
	}
	if (pickedItemCount == 0)
	{
		*pickedItems = NULL;
		return;
	}

	*pickedItems = VuoListCreateWithCount_VuoGenericType1(pickedItemCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *pickedItemsData = VuoListGetData_VuoGenericType1(*pickedItems);

	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);
	picked = 0;
	skipped = 0;
	unsigned long m = 0;
	for (unsigned long i = 0; i < count; ++i)
	{
		if (picked < pickCount)
			++picked;
		else
		{
			if (skipped < skipCountClamped)
			{
				++skipped;
				continue;
			}
			else
			{
				skipped = 0;
				picked = 1;
			}
		}

		pickedItemsData[m] = listData[i];
		VuoGenericType1_retain(pickedItemsData[m++]);
	}
}
