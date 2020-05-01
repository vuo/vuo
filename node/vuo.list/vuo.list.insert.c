/**
 * @file
 * vuo.list.insert node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Insert in List",
					 "keywords" : [ "push", "append", "prepend", "insert", "combine", "place", "middle", "add" ],
					 "version" : "1.0.1",
					 "node": {
						 "exampleCompositions": [ "InsertSquare.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1}) position,
		VuoInputData(VuoGenericType1) item,
		VuoOutputData(VuoList_VuoGenericType1) modifiedList
)
{
	if (list)
		*modifiedList = VuoListCopy_VuoGenericType1(list);
	else
		*modifiedList = VuoListCreate_VuoGenericType1();
	VuoListInsertValue_VuoGenericType1(*modifiedList, item, MAX(position, 0));
}
