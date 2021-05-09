/**
 * @file
 * vuo.list.get node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Item from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index" ],
					 "version" : "1.0.1",
					 "node": {
						  "exampleCompositions" : [ "SelectLayerFromList.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1}) which,
		VuoOutputData(VuoGenericType1) item
)
{
	unsigned long whichUnsigned = MAX(0, which);
	
	unsigned long listSize = VuoListGetCount_VuoGenericType1(list);
	unsigned long index = VuoListIndexToCArrayIndex(whichUnsigned, listSize);
	*item = VuoListGetData_VuoGenericType1(list)[index];
}
