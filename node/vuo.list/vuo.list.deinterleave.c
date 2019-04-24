/**
 * @file
 * vuo.list.deinterleave node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Deinterleave Lists",
					 "keywords" : [
						 "split", "separate",
						 "alternate", "distribute", "take turns", "rotate",
					 ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoGenericType1) interleavedList,
	VuoOutputData(VuoList_VuoGenericType1) list1,
	VuoOutputData(VuoList_VuoGenericType1) list2
)
{
	unsigned long count = VuoListGetCount_VuoGenericType1(interleavedList);
	VuoGenericType1 *interleavedData = VuoListGetData_VuoGenericType1(interleavedList);

	*list1 = VuoListCreateWithCount_VuoGenericType1(count / 2 + count % 2, VuoGenericType1_makeFromJson(NULL));
	*list2 = VuoListCreateWithCount_VuoGenericType1(count / 2, VuoGenericType1_makeFromJson(NULL));

	VuoGenericType1 *listData[2];
	listData[0] = VuoListGetData_VuoGenericType1(*list1);
	listData[1] = VuoListGetData_VuoGenericType1(*list2);

	int which = 0;
	for (unsigned long i = 0; i < count; ++i, ++which)
	{
		listData[which % 2][i / 2] = interleavedData[i];
		VuoGenericType1_retain(listData[which % 2][i / 2]);
	}
}
