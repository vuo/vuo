/**
 * @file
 * vuo.event.allowFirst node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Allow First Event",
					  "keywords" : [ "filter", "hold", "block", "prevent", "pass", "once", "single", "start", "initialize" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "SpinKaleidoscope.vuo" ]
					  }
				  });

VuoBoolean * nodeInstanceInit()
{
	VuoBoolean *receivedEvent = (VuoBoolean *)malloc(sizeof(VuoBoolean));
	VuoRegister(receivedEvent, free);
	*receivedEvent = false;
	return receivedEvent;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoBoolean *) receivedEvent,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":true}) event,
		VuoInputEvent({"eventBlocking":"wall"}) reset,
		VuoOutputEvent() firstEvent
)
{
	if(reset)
	{
		**receivedEvent = false;
	}

	if( event && !**receivedEvent )
	{
		*firstEvent = event;
		**receivedEvent = true;
	}
}

void nodeInstanceFini
(
		VuoInstanceData(VuoBoolean *) receivedEvent
)
{
}
