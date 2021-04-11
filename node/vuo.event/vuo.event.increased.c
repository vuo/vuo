/**
 * @file
 * vuo.event.increased node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Increased",
					  "keywords" : [ "pulse", "watcher", "changed" ],
					  "version" : "1.1.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [
								  /* Sync with vuo.event.decreased2 */
								  "VuoInteger", /* VuoInteger first, so this node can be compiled when unspecialized, since it's a core type */
								  "VuoArtNetInputDevice",
								  "VuoArtNetOutputDevice",
								  "VuoBaudRate",
								  "VuoColor",
								  "VuoData",
								  "VuoDistribution3d",
								  "VuoDragEvent",
								  "VuoFont",
								  "VuoGridType",
								  "VuoHidControl",
								  "VuoHidDevice",
								  "VuoImageFormat",
								  "VuoMidiPitchBend",
								  "VuoMultisample",
								  "VuoNumberFormat",
								  "VuoOscInputDevice",
								  "VuoOscOutputDevice",
								  "VuoParity",
								  "VuoRange",
								  "VuoReal",
								  "VuoRectangle",
								  "VuoSerialDevice",
								  "VuoText",
								  "VuoTimeUnit",
								  "VuoTime",
								  "VuoUrl",
								  "VuoVertexAttribute",
								  "VuoWeekday",
							  ]
						  }
					  },
					  "node" : {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
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
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"door","data":"value"}) valueEvent,
		VuoOutputEvent() increased
)
{
	*increased = VuoGenericType1_isLessThan(**lastValue, value);
	**lastValue = value;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoGenericType1 *) lastValue
)
{
}
