/**
 * @file
 * vuo.data.hold node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Hold Value",
					 "keywords" : [ "store", "retain", "keep", "sample", "preserve", "feedback", "loop", "control flow", "block", "wall" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "StoreMousePosition.vuo" ]
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
		VuoInputEvent({"eventBlocking":"wall","data":"initialValue"}) initialValueEvent,
		VuoInputData(VuoGenericType1) newValue,
		VuoInputEvent({"eventBlocking":"wall","data":"newValue","hasPortAction":true}) newValueEvent,
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
