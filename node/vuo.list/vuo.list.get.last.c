/**
 * @file
 * vuo.list.get.last node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Last Item in List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "back", "final", "end" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoOutputData(VuoGenericType1) lastItem
)
{
	*lastItem = VuoListGetValue_VuoGenericType1(list, VuoListGetCount_VuoGenericType1(list));
}
