/**
 * @file
 * vuo.type.transform.event node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
		VuoInputData(VuoTransform) dataAndEvent,
		VuoInputEvent(VuoPortEventBlocking_None,dataAndEvent) inEvent,
		VuoOutputEvent() event
)
{
	*event = inEvent;
}
