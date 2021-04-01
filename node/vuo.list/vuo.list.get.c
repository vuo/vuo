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
	
	if(whichUnsigned < 1)
		*item = VuoListGetData_VuoGenericType1(list)[0];
	else if(whichUnsigned > VuoListGetCount_VuoGenericType1(list))
		*item = VuoListGetData_VuoGenericType1(list)[VuoListGetCount_VuoGenericType1(list) - 1];
	else
		*item = VuoListGetData_VuoGenericType1(list)[whichUnsigned - 1];
}
