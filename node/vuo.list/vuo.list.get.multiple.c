/**
 * @file
 * vuo.list.get.multiple node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Items from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index", "indices" ],
					 "version" : "1.0.0",
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
	if(!positions)
		return;

	*items = VuoListCreate_VuoGenericType1();

	for(int i = 1; i <= VuoListGetCount_VuoInteger(positions); i++)
	{
		VuoInteger index = VuoListGetValue_VuoInteger(positions, i);
		unsigned long indexUnsigned = MAX(0, index);
		VuoListAppendValue_VuoGenericType1(*items, VuoListGetValue_VuoGenericType1(list, indexUnsigned));
	}
}
