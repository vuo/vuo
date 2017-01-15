/**
 * @file
 * vuo.list.add node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoListPosition.h"

VuoModuleMetadata({
					 "title" : "Add to List",
					 "keywords" : [ "push", "append", "prepend", "insert", "combine" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ "ShiftSquares.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoListPosition, {"default":"end"}) position,
		VuoInputData(VuoGenericType1) item,
		VuoOutputData(VuoList_VuoGenericType1) modifiedList
)
{
	*modifiedList = VuoListCopy_VuoGenericType1(list);
	if (position == VuoListPosition_Beginning)
		VuoListPrependValue_VuoGenericType1(*modifiedList, item);
	else
		VuoListAppendValue_VuoGenericType1(*modifiedList, item);
}
