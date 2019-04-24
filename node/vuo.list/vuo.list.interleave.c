/**
 * @file
 * vuo.list.interleave node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Interleave Lists",
					 "keywords" : [
						 "combine", "join", "together", "merge",
						 "alternate", "intersperse", "spread", "take turns", "rotate",
					 ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ "vuo-example://vuo.image/CompareNoiseTypes.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoGenericType1) list1,
	VuoInputData(VuoList_VuoGenericType1) list2,
	VuoOutputData(VuoList_VuoGenericType1) interleavedList
)
{
	unsigned long count1 = VuoListGetCount_VuoGenericType1(list1);
	unsigned long count2 = VuoListGetCount_VuoGenericType1(list2);
	if (!count1 && !count2)
	{
		*interleavedList = VuoListCreate_VuoGenericType1();
		return;
	}
	if (!count1)
	{
		*interleavedList = list2;
		return;
	}
	if (!count2)
	{
		*interleavedList = list1;
		return;
	}

	unsigned long count = MAX(count1, count2);
	*interleavedList = VuoListCreateWithCount_VuoGenericType1(count * 2, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *interleavedData = VuoListGetData_VuoGenericType1(*interleavedList);
	VuoGenericType1 *data1 = VuoListGetData_VuoGenericType1(list1);
	VuoGenericType1 *data2 = VuoListGetData_VuoGenericType1(list2);

	for (unsigned long i = 0; i < count; ++i)
	{
		interleavedData[i * 2    ] = data1[MIN(count1 - 1, i)];
		interleavedData[i * 2 + 1] = data2[MIN(count2 - 1, i)];
		VuoGenericType1_retain(interleavedData[i * 2    ]);
		VuoGenericType1_retain(interleavedData[i * 2 + 1]);
	}
}
