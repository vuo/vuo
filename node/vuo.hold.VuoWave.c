/**
 * @file
 * vuo.hold.VuoWave node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Hold Value",
					 "description" :
						"<p>When its refresh port receives an event, this node outputs its stored value.</p> \
						<p>Before the `newValue` port has received an event, this node stores the `initialValue` port's value. \
						Typically, you would want to set a constant value on the `initialValue` port rather than connecting a cable.</p> \
						<p>When the `newValue` port receives an event, this node replaces its stored value with the `newValue` port's current value.</p> \
						<p>This node is useful in feedback loops and other situations where you want to store a value with one event \
						and later output it with another event.</p>",
					 "keywords" : [ "store", "sample", "feedback", "loop", "control flow", "block", "wall" ],
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
		VuoInputData(VuoWave) initialValue,
		VuoInputEvent(VuoPortEventBlocking_Wall,initialValue) initialValueEvent,
		VuoInputData(VuoWave) newValue,
		VuoInputEvent(VuoPortEventBlocking_Wall,newValue) newValueEvent,
		VuoOutputData(VuoWave) heldValue
)
{
	if (newValueEvent)
		**hasNewValueReceivedEvent = true;

	*heldValue = (**hasNewValueReceivedEvent ? newValue : initialValue);
}

void nodeInstanceFini(VuoInstanceData(bool *) hasNewValueReceivedEvent)
{
}
