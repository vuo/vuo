/**
 * @file
 * vuo.list.insert node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Insert in List",
					 "keywords" : [ "push", "append", "prepend", "insert", "combine", "place", "middle" ],
					 "version" : "1.0.0",
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
	*modifiedList = VuoListCopy_VuoGenericType1(list);
	VuoListInsertValue_VuoGenericType1(*modifiedList, item, MAX(position, 0));
}
