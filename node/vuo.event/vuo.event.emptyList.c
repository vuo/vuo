/**
 * @file
 * vuo.event.emptyList node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Block Empty List",
					 "keywords" : [ "keep", "filter", "filled", "items", "count" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputEvent(VuoPortEventBlocking_Door, list, {"hasPortAction":true}) listEvent,
		VuoOutputData(VuoList_VuoGenericType1) nonEmptyList,
		VuoOutputEvent(nonEmptyList) nonEmptyListEvent
)
{
	if ( VuoListGetCount_VuoGenericType1(list) > 0 )
	{
		*nonEmptyList = list;
		*nonEmptyListEvent = true;
	}
}
