/**
 * @file
 * vuo.list.get.random node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Random Item from List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "any" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoOutputData(VuoGenericType1) randomItem
)
{
	unsigned long count = VuoListGetCount_VuoGenericType1(list);
	*randomItem = VuoListGetValue_VuoGenericType1(list, VuoInteger_random(1, count));
}
