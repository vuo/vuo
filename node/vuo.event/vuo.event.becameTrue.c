/**
 * @file
 * vuo.event.becameTrue node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Became True",
					 "keywords" : [ "pulse", "watcher", "change" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

VuoBoolean * nodeInstanceInit(void)
{
	VuoBoolean *lastValue = (VuoBoolean *)malloc(sizeof(VuoBoolean));
	VuoRegister(lastValue, free);
	*lastValue = false;
	return lastValue;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoBoolean *) lastValue,
		VuoInputData(VuoBoolean, {"default":false}) value,
		VuoInputEvent(VuoPortEventBlocking_Door, value) valueEvent,
		VuoOutputEvent() becameTrue
)
{
	*becameTrue = (**lastValue == false && value == true);
	**lastValue = value;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoBoolean *) lastValue
)
{
}
