/**
 * @file
 * vuo.midi.listDevices node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "List MIDI Devices",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoList_VuoMidiInputDevice) inputDevices,
		VuoOutputData(VuoList_VuoMidiOutputDevice) outputDevices
)
{
	*inputDevices = VuoMidi_getInputDevices();
	*outputDevices = VuoMidi_getOutputDevices();
}
