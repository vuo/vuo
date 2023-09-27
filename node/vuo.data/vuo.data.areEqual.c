﻿/**
 * @file
 * vuo.data.areEqual node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Are Equal (Generic)",
					  "keywords" : [ "==", "same", "identical", "equivalent", "match", "compare" ],
					  "version" : "1.0.0",
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
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) values,
		VuoOutputData(VuoBoolean) equal
)
{
	*equal = true;
	unsigned long count = VuoListGetCount_VuoGenericType1(values);
	for (unsigned long i = 2; i <= count && *equal; ++i)
		if (! VuoGenericType1_areEqual(VuoListGetValue_VuoGenericType1(values, i - 1), VuoListGetValue_VuoGenericType1(values, i)))
			*equal = false;
}
