/**
 * @file
 * vuo.event.increased node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Increased",
					  "keywords" : [ "pulse", "watcher", "changed" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
					  },
					  "node" : {
						  "isInterface" : false
					  }
				  });

VuoGenericType1 * nodeInstanceInit( VuoInputData(VuoGenericType1) value )
{
	VuoGenericType1 *lastValue = (VuoGenericType1 *)malloc(sizeof(VuoGenericType1));
	VuoRegister(lastValue, free);
	*lastValue = value;
	return lastValue;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoGenericType1 *) lastValue,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) value,
		VuoInputEvent(VuoPortEventBlocking_Door, value) valueEvent,
		VuoOutputEvent() increased
)
{
	*increased = (value > **lastValue);
	**lastValue = value;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoGenericType1 *) lastValue
)
{
}
