/**
 * @file
 * vuo.midi.send node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "Send MIDI Event",
					 "description" :
						"<p>Sends a MIDI message to a device.</p> \
						<p>This node can be used to control synthesizers, sequencers, and other musical software and hardware, \
						as well as stage lighting and other performance equipment.</p> \
						<p>This node does not make sound on its own. It requires MIDI hardware or software.</p> \
						<p><ul> \
						<li>`device` — The device to send to. If no device is given, then the first available MIDI output device is used.</li> \
						<li>`sendNote` — When this port receives an event, the note message is sent to the device.</li> \
						<li>`sendController` — When this port receives an event, the controller message is sent to the device.</li> \
						</ul></p>",
					 "keywords" : [ "note", "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "isInterface" : true
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
