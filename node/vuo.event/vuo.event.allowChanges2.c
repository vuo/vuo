/**
 * @file
 * vuo.event.allowChanges2 node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Allow Changes",
					  "keywords" : [ "pulse", "watcher", "filter", "hold", "block", "prevent", "unchanged", "same", "duplicate", "pass" ],
					  "version" : "2.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [
								  /* Sync with vuo.event.changed2 */
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
								  "VuoRectangle",
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
						  "exampleCompositions" : [ ]
					  }
				  });

struct nodeInstanceData
{
	bool first;
	VuoGenericType1 value;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->first = true;
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"door","data":"value"}) valueEvent,
		VuoOutputData(VuoGenericType1) changedValue,
		VuoOutputEvent({"data":"changedValue"}) changedValueEvent
)
{
	if ((*context)->first)
	{
		*changedValue = value;
		*changedValueEvent = true;

		(*context)->first = false;
		(*context)->value = value;
		VuoGenericType1_retain((*context)->value);
		return;
	}

	if (!VuoGenericType1_areEqual((*context)->value, value))
	{
		*changedValue = value;
		*changedValueEvent = true;

		VuoGenericType1_release((*context)->value);
		(*context)->value = value;
		VuoGenericType1_retain((*context)->value);
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if (!(*context)->first)
		VuoGenericType1_release((*context)->value);
}
