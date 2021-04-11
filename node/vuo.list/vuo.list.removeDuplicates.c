/**
 * @file
 * vuo.list.removeDuplicates node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Remove Duplicates",
	"keywords" : [
		"deduplicate items",
		"delete",
		"make unique",
	],
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
				"VuoHidControl", "VuoHidDevice",
				"VuoImageFormat",
				"VuoFont", "VuoMidiController", "VuoMidiInputDevice",
				"VuoGridType",
				"VuoMidiNote", "VuoMidiOutputDevice", "VuoMidiPitchBend",
				"VuoMultisample", "VuoBaudRate", "VuoParity",
				"VuoOscInputDevice", "VuoOscOutputDevice",
				"VuoRange",
				"VuoRectangle",
				"VuoRelativeTime", "VuoRoundingMethod",
				"VuoSerialDevice", "VuoVertexAttribute", "VuoSyphonServerDescription",
				"VuoTempoRange", "VuoNumberFormat", "VuoMovieFormat",
				"VuoVideoFrame", "VuoVideoInputDevice",
				"VuoTime", "VuoTimeUnit", "VuoTimeFormat", "VuoWeekday",
				"VuoHorizontalAlignment", "VuoVerticalAlignment", "VuoAnchor",
			],
		},
	},
	"node": {
		"exampleCompositions" : [ "vuo-example://vuo.hid/ListKeypresses.vuo" ]
	}
});

void nodeEvent
(
	VuoInputData(VuoList_VuoGenericType1) list,
	VuoOutputData(VuoList_VuoGenericType1) uniqueList
)
{
	*uniqueList = VuoListRemoveDuplicates_VuoGenericType1(list);
}
