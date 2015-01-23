/**
 * @file
 * vuo.type.discardData node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Discard Data from Event",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoGenericType1) dataAndEvent,
		VuoInputEvent(VuoPortEventBlocking_None,dataAndEvent) inEvent,
		VuoOutputEvent() event
)
{
	*event = inEvent;
}
