/**
 * @file
 * vuo.list.get.multiple node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Items from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index", "indices", "reorder", "rearrange", "shuffle", "combination" ],
					 "version" : "1.0.1",
					 "node": {
						  "exampleCompositions" : [ "SelectLayerFromList.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoList_VuoInteger) positions,
		VuoOutputData(VuoList_VuoGenericType1) items
)
{
	if (VuoListGetCount_VuoInteger(positions) == 0)
	{
		*items = NULL;
		return;
	}

	*items = VuoListCreate_VuoGenericType1();
	
	VuoGenericType1* listCopy = VuoListGetData_VuoGenericType1(list);
	VuoInteger* positionsList = VuoListGetData_VuoInteger(positions);
	for(int i = 1; i <= VuoListGetCount_VuoInteger(positions); i++)
	{
		VuoInteger index = positionsList[i - 1];
		unsigned long indexUnsigned = MAX(0, index);

		if(indexUnsigned < 1)
			VuoListAppendValue_VuoGenericType1(*items, listCopy[0]);
		else if(indexUnsigned > VuoListGetCount_VuoInteger(list))
			VuoListAppendValue_VuoGenericType1(*items, listCopy[VuoListGetCount_VuoInteger(list) - 1]);
		else
			VuoListAppendValue_VuoGenericType1(*items, listCopy[indexUnsigned - 1]);
	}
}
