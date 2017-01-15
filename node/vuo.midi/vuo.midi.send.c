/**
 * @file
 * vuo.midi.send node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "Send MIDI Event",
					 "keywords" : [ "note", "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "2.1.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "SendMidiNotes.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoMidiOutputDevice device;
	VuoMidiOut midiManager;
};

static void updateDevice(struct nodeInstanceData *context, VuoMidiOutputDevice newDevice)
{
	VuoMidiOutputDevice_release(context->device);
	context->device = newDevice;
	VuoMidiOutputDevice_retain(context->device);

	VuoRelease(context->midiManager);
	context->midiManager = VuoMidiOut_make(newDevice);
	VuoRetain(context->midiManager);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoMidiOutputDevice) device
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	updateDevice(context, device);
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoMidiOutputDevice) device,
		VuoInputData(VuoMidiNote) sendNote,
		VuoInputEvent({"eventBlocking":"none","data":"sendNote"}) sendNoteEvent,
		VuoInputData(VuoMidiController) sendController,
		VuoInputEvent({"eventBlocking":"none","data":"sendController"}) sendControllerEvent,
		VuoInputData(VuoMidiPitchBend) sendPitchBend,
		VuoInputEvent({"eventBlocking":"none","data":"sendPitchBend"}) sendPitchBendEvent
//		VuoInputData(VuoMidiAftertouch,"") aftertouch,
//		VuoInputData(VuoMidiCommand,"") command,
//		VuoInputData(VuoMidiClock,"") clock,
//		VuoInputData(VuoMidiTimecode,"") timecode,
//		VuoInputData(VuoMidiSysEx,"") sysEx
)
{
	if (! VuoMidiOutputDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device);

	if (sendNoteEvent)
		VuoMidiOut_sendNote((*context)->midiManager, sendNote);
	if (sendControllerEvent)
		VuoMidiOut_sendController((*context)->midiManager, sendController);
	if (sendPitchBendEvent)
		VuoMidiOut_sendPitchBend((*context)->midiManager, sendPitchBend);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoMidiOutputDevice_release((*context)->device);
	VuoRelease((*context)->midiManager);
}
