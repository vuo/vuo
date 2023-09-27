/**
 * @file
 * vuo.midi.listDevices2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "List MIDI Devices",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "3.0.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void *nodeInstanceInit()
{
	VuoMidi_use();
	return NULL;
}

void nodeInstanceTriggerStart
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoMidiInputDevice),
	VuoOutputTrigger(outputDevices, VuoList_VuoMidiOutputDevice)
)
{
	VuoMidi_addDevicesChangedTriggers(inputDevices, outputDevices);
}

void nodeInstanceEvent
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoMidiInputDevice),
	VuoOutputTrigger(outputDevices, VuoList_VuoMidiOutputDevice)
)
{
}

void nodeInstanceTriggerStop
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(inputDevices, VuoList_VuoMidiInputDevice),
	VuoOutputTrigger(outputDevices, VuoList_VuoMidiOutputDevice)
)
{
	VuoMidi_removeDevicesChangedTriggers(inputDevices, outputDevices);
}

void nodeInstanceFini
(
	VuoInstanceData(void *) nodeInstanceData
)
{
	VuoMidi_disuse();
}
