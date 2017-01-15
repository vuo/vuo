/**
 * @file
 * vuo.event.emptyList node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Block Empty List",
					 "keywords" : [ "keep", "filter", "pass", "filled", "items", "count", "non-empty", "nonempty" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputEvent({"eventBlocking":"door","data":"list","hasPortAction":true}) listEvent,
		VuoOutputData(VuoList_VuoGenericType1) nonEmptyList,
		VuoOutputEvent({"data":"nonEmptyList"}) nonEmptyListEvent
)
{
	if ( VuoListGetCount_VuoGenericType1(list) > 0 )
	{
		*nonEmptyList = list;
		*nonEmptyListEvent = true;
	}
}
