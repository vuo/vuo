/**
 * @file
 * vuo.logic.switch node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Switch",
					 "keywords" : [ "boolean", "gate", "not", "!", "flip", "inverse", "reverse", "opposite", "toggle" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

VuoBoolean * nodeInstanceInit(void)
{
	VuoBoolean *state = (VuoBoolean *)malloc(sizeof(VuoBoolean));
	VuoRegister(state, free);
	*state = false;
	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoBoolean *) state,
		VuoInputEvent(VuoPortEventBlocking_None, ) toggle,
		VuoInputEvent(VuoPortEventBlocking_None, ) turnOn,
		VuoInputEvent(VuoPortEventBlocking_None, ) turnOff,
		VuoOutputData(VuoBoolean) value
)
{
	if (toggle)
		**state = ! **state;

	if (turnOn)
		**state = true;

	if (turnOff)
		**state = false;

	*value = **state;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoBoolean *) state
)
{
}
