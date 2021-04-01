/**
 * @file
 * vuo.list.comb node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
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
					 "version" : "1.0.0",
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
	*pickedItems = VuoListCreate_VuoGenericType1();

	unsigned long count = VuoListGetCount_VuoGenericType1(list);
	if (!count)
		return;

	unsigned long picked = 0;
	unsigned long skipped = 0;
	VuoGenericType1* listCopy = VuoListGetData_VuoGenericType1(list);
	for (unsigned long i = 1; i <= count; ++i)
	{
		if (picked < pickCount)
			++picked;
		else
		{
			if (skipped < skipCount)
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

		VuoListAppendValue_VuoGenericType1(*pickedItems, listCopy[i-1]);
	}
}
