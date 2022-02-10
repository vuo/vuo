/**
 * @file
 * vuo.event.changed node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Changed",
					  "keywords" : [ "pulse", "watcher" ],
					  "version" : "1.2.1",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [
								  /* Sync with other nodes that say "Sync with vuo.event.changed" */
								  "VuoBoolean", "VuoColor", "VuoImage", "VuoInteger",
								  "VuoPoint2d", "VuoPoint3d", "VuoPoint4d",
								  "VuoReal", "VuoScreen", "VuoText",
								  "VuoArtNetInputDevice", "VuoArtNetOutputDevice",
								  "VuoAudioInputDevice", "VuoAudioOutputDevice", "VuoAudioFrame", "VuoData",
								  "VuoCoordinateUnit", "VuoDistribution3d", "VuoDragEvent",
								  "VuoGridType",
								  "VuoHidControl", "VuoHidDevice",
								  "VuoImageFormat",
								  "VuoFont", "VuoMidiController", "VuoMidiInputDevice",
								  "VuoMidiNote", "VuoMidiOutputDevice", "VuoMidiPitchBend",
								  "VuoMultisample", "VuoBaudRate", "VuoParity",
								  "VuoOscInputDevice", "VuoOscOutputDevice",
								  "VuoRange",
								  "VuoRelativeTime", "VuoRoundingMethod",
								  "VuoSerialDevice", "VuoVertexAttribute", "VuoSyphonServerDescription",
								  "VuoTempoRange", "VuoNumberFormat", "VuoIconPosition", "VuoMovieFormat",
								  "VuoVideoFrame", "VuoVideoInputDevice",
								  "VuoTime", "VuoTimeUnit", "VuoTimeFormat", "VuoWeekday",
								  "VuoHorizontalAlignment", "VuoVerticalAlignment", "VuoAnchor"
							  ]
						  }
					  },
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
					  }
				  });

VuoGenericType1 * nodeInstanceInit
(
	VuoInputData(VuoGenericType1) value
)
{
	VuoGenericType1 *lastValue = (VuoGenericType1 *)malloc(sizeof(VuoGenericType1));
	VuoRegister(lastValue, free);
	*lastValue = value;
	VuoGenericType1_retain(*lastValue);
	return lastValue;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoGenericType1 *) lastValue,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"door","data":"value"}) valueEvent,
		VuoOutputEvent() changed
)
{
	*changed = (! VuoGenericType1_areEqual(**lastValue, value));
	VuoGenericType1_release(**lastValue);
	**lastValue = value;
	VuoGenericType1_retain(**lastValue);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoGenericType1 *) lastValue
)
{
	VuoGenericType1_release(**lastValue);
}
