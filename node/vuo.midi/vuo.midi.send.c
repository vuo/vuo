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


struct nodeInstanceData
{
	VuoMidiDevice device;
	VuoMidiOut midiManager;
};

static void updateDevice(struct nodeInstanceData *context, VuoMidiDevice newDevice)
{
	VuoMidiDevice_release(context->device);
	context->device = newDevice;
	VuoMidiDevice_retain(context->device);

	VuoRelease(context->midiManager);
	context->midiManager = VuoMidiOut_make(newDevice);
	VuoRetain(context->midiManager);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoMidiDevice) device
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
		VuoInputData(VuoMidiDevice, {"default":{"isInput":false}}) device,
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
	if (! VuoMidiDevice_areEqual(device, (*context)->device))
		updateDevice(*context, device);

	if (sendNoteEvent)
		VuoMidiOut_sendNote((*context)->midiManager, sendNote);
	if (sendControllerEvent)
		VuoMidiOut_sendController((*context)->midiManager, sendController);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoMidiDevice_release((*context)->device);
	VuoRelease((*context)->midiManager);
}
