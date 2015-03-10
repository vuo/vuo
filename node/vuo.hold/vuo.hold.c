/**
 * @file
 * vuo.hold node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Hold Value",
					 "keywords" : [ "store", "retain", "keep", "sample", "feedback", "loop", "control flow", "block", "wall" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

bool * nodeInstanceInit()
{
	bool *hasNewValueReceivedEvent = (bool *) malloc(sizeof(bool));
	VuoRegister(hasNewValueReceivedEvent, free);
	*hasNewValueReceivedEvent = false;
	return hasNewValueReceivedEvent;
}

void nodeInstanceEvent
(
		VuoInstanceData(bool *) hasNewValueReceivedEvent,
		VuoInputData(VuoGenericType1) initialValue,
		VuoInputEvent(VuoPortEventBlocking_Wall,initialValue) initialValueEvent,
		VuoInputData(VuoGenericType1) newValue,
		VuoInputEvent(VuoPortEventBlocking_Wall,newValue) newValueEvent,
		VuoOutputData(VuoGenericType1) heldValue
)
{
	if (newValueEvent)
		**hasNewValueReceivedEvent = true;

	*heldValue = (**hasNewValueReceivedEvent ? newValue : initialValue);
}

void nodeInstanceFini(VuoInstanceData(bool *) hasNewValueReceivedEvent)
{
}
