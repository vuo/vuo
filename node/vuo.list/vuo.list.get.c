/**
 * @file
 * vuo.list.get node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Item from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "index" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1}) which,
		VuoOutputData(VuoGenericType1) item
)
{
	*item = VuoListGetValue_VuoGenericType1(list, which);
}
