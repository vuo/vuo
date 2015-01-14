/**
 * @file
 * vuo.type.real.event node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Discard Data from Event",
					 "description" :
						 "<p>When this node receives an event (possibly with data), it outputs the event (without data).</p> \
						 <p>This node is useful for connecting a data-and-event output port to an event-only input port.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoVertices) dataAndEvent,
		VuoInputEvent(VuoPortEventBlocking_None,dataAndEvent) inEvent,
		VuoOutputEvent() event
)
{
	*event = inEvent;
}
