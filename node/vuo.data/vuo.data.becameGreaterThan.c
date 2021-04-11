/**
 * @file
 * vuo.data.becameGreaterThan node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Became Greater Than",
	"keywords": [
		"pulse", "watcher", "change",
		"large", "big", "high", "more", "most", ">", "compare", "conditional", "after",
	],
	"version": "1.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"compatibleTypes": [
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
	"node": {
		"exampleCompositions" : [ ]
	}
});

bool nodeInstanceInit(void)
{
	return false;
}

void nodeInstanceEvent(
	VuoInstanceData(bool) lastStatus,
	VuoInputData(VuoGenericType1) a,
	VuoInputData(VuoGenericType1) b,
	VuoInputEvent({"eventBlocking":"door","data":"a"}) aEvent,
	VuoInputEvent({"eventBlocking":"door","data":"b"}) bEvent,
	VuoOutputEvent() becameGreaterThan)
{
	bool currentlyGreaterThan = VuoGenericType1_isLessThan(b, a);
	*becameGreaterThan = *lastStatus == false && currentlyGreaterThan;
	*lastStatus = currentlyGreaterThan;
}
