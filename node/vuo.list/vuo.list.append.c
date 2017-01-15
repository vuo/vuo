/**
 * @file
 * vuo.list.append node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Append Lists",
					 "keywords" : [ "push", "append", "prepend", "insert", "combine", "concatenate", "join", "together", "merge" ],
					 "version" : "1.0.0",
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
	*combinedList = VuoListCreate_VuoGenericType1();

	unsigned long list1Count = VuoListGetCount_VuoGenericType1(list1);
	for (unsigned long i = 1; i <= list1Count; ++i)
		VuoListAppendValue_VuoGenericType1(*combinedList, VuoListGetValue_VuoGenericType1(list1, i));

	unsigned long list2Count = VuoListGetCount_VuoGenericType1(list2);
	for (unsigned long i = 1; i <= list2Count; ++i)
		VuoListAppendValue_VuoGenericType1(*combinedList, VuoListGetValue_VuoGenericType1(list2, i));
}
