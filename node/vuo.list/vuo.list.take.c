﻿/**
 * @file
 * vuo.list.take node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Take from List",
					 "keywords" : [ "pop", "remove", "delete", "extract", "truncate", "trim", "sublist", "cut" ],
					 "version" : "1.0.1",
					 "node" : {
						  "exampleCompositions" : [ "ShiftSquares.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoListPosition, {"default":"beginning"}) position,
		VuoOutputData(VuoGenericType1) removedItem,
		VuoOutputData(VuoList_VuoGenericType1) modifiedList
)
{
	if (!list)
	{
		*removedItem = VuoGenericType1_makeFromJson(NULL);
		*modifiedList = VuoListCreate_VuoGenericType1();
		return;
	}

	*modifiedList = VuoListCopy_VuoGenericType1(list);
	if (position == VuoListPosition_Beginning)
	{
		*removedItem = VuoListGetValue_VuoGenericType1(*modifiedList, 1);
		VuoListRemoveFirstValue_VuoGenericType1(*modifiedList);
	}
	else
	{
		*removedItem = VuoListGetValue_VuoGenericType1(*modifiedList, VuoListGetCount_VuoGenericType1(*modifiedList));
		VuoListRemoveLastValue_VuoGenericType1(*modifiedList);
	}
}
