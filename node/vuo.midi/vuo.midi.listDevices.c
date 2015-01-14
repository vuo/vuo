/**
 * @file
 * vuo.midi.listDevices node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "List MIDI Devices",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "isInterface" : true
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoList_VuoMidiDevice) inputDevices,
		VuoOutputData(VuoList_VuoMidiDevice) outputDevices
)
{
	*inputDevices = VuoMidi_getInputDevices();
	*outputDevices = VuoMidi_getOutputDevices();
}
