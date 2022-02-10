/**
 * @file
 * vuo.list.append node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Append Lists",
					 "keywords" : [ "push", "append", "prepend", "insert", "combine", "concatenate", "join", "together", "merge" ],
					 "version" : "1.0.1",
					 "node" : {
						  "exampleCompositions" : [ "SpliceSquares.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list1,
		VuoInputData(VuoList_VuoGenericType1) list2,
		VuoOutputData(VuoList_VuoGenericType1) combinedList
)
{
	unsigned long list1Count = VuoListGetCount_VuoGenericType1(list1);
	unsigned long list2Count = VuoListGetCount_VuoGenericType1(list2);
	*combinedList = VuoListCreateWithCount_VuoGenericType1(list1Count + list2Count, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *combinedListData = VuoListGetData_VuoGenericType1(*combinedList);

	VuoGenericType1 *list1Data = VuoListGetData_VuoGenericType1(list1);
	for (unsigned long i = 0; i < list1Count; ++i)
	{
		combinedListData[i] = list1Data[i];
		VuoGenericType1_retain(combinedListData[i]);
	}

	VuoGenericType1 *list2Data = VuoListGetData_VuoGenericType1(list2);
	for (unsigned long i = 0; i < list2Count; ++i)
	{
		combinedListData[list1Count + i] = list2Data[i];
		VuoGenericType1_retain(combinedListData[list1Count + i]);
	}
}
