/**
 * @file
 * vuo.midi.send node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "Send MIDI Event",
					 "keywords" : [ "note", "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "SendMidiNotes.vuo" ]
					 }
				 });


VuoMidiOut nodeInstanceInit(
		VuoInputData(VuoMidiDevice, {"default":{"isInput":false}}) device
)
{
	VuoMidiOut mo = VuoMidiOut_make(device);
	VuoRetain(mo);
	return mo;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoMidiOut) context,
		VuoInputData(VuoMidiDevice, {"default":{"isInput":false}}) device,
		VuoInputEvent(VuoPortEventBlocking_Wall, device) deviceEvent,
		VuoInputData(VuoMidiNote,"") sendNote,
		VuoInputEvent(VuoPortEventBlocking_None, sendNote) sendNoteEvent,
		VuoInputData(VuoMidiController,"") sendController,
		VuoInputEvent(VuoPortEventBlocking_None, sendController) sendControllerEvent
//		VuoInputData(VuoMidiAftertouch,"") aftertouch,
//		VuoInputData(VuoMidiCommand,"") command,
//		VuoInputData(VuoMidiClock,"") clock,
//		VuoInputData(VuoMidiTimecode,"") timecode,
//		VuoInputData(VuoMidiSysEx,"") sysEx
)
{
	if (deviceEvent)
	{
		VuoRelease(*context);
		*context = VuoMidiOut_make(device);
		VuoRetain(*context);
	}

	if (sendNoteEvent)
		VuoMidiOut_sendNote(*context, sendNote);
	if (sendControllerEvent)
		VuoMidiOut_sendController(*context, sendController);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoMidiOut) context
)
{
	VuoRelease(*context);
}
