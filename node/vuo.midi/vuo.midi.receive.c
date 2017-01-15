/**
 * @file
 * vuo.midi.receive node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "Receive MIDI Events",
					 "keywords" : [ "note", "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "2.1.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "ReceiveMidiNotes.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoMidiInputDevice device;
	VuoMidiIn midiManager;
	void (*receivedNote)(VuoMidiNote);
	void (*receivedController)(VuoMidiController);
	void (*receivedPitchBend)(VuoMidiPitchBend);
};

static void updateDevice(struct nodeInstanceData *context, VuoMidiInputDevice newDevice)
{
	VuoMidiInputDevice_release(context->device);
	context->device = newDevice;
	VuoMidiInputDevice_retain(context->device);

	VuoRelease(context->midiManager);
	context->midiManager = VuoMidiIn_make(newDevice);
	VuoRetain(context->midiManager);
}

static void receivedNoteWrapper(void *context, VuoMidiNote note)
{
	struct nodeInstanceData *c = (struct nodeInstanceData *)context;
	c->receivedNote(note);
}

static void receivedControllerWrapper(void *context, VuoMidiController controller)
{
	struct nodeInstanceData *c = (struct nodeInstanceData *)context;
	c->receivedController(controller);
}

static void receivedPitchBendWrapper(void *context, VuoMidiPitchBend pitchBend)
{
	struct nodeInstanceData *c = (struct nodeInstanceData *)context;
	c->receivedPitchBend(pitchBend);
}

struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoMidiInputDevice, {"default":{"isInput":true}}) device
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	updateDevice(context, device);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedNote, VuoMidiNote),
		VuoOutputTrigger(receivedController, VuoMidiController),
		VuoOutputTrigger(receivedPitchBend, VuoMidiPitchBend)
//		VuoOutputTrigger(receivedAftertouch, VuoMidiAftertouch),
//		VuoOutputTrigger(receivedCommand, VuoMidiCommand),
//		VuoOutputTrigger(receivedClock, VuoMidiClock),
//		VuoOutputTrigger(receivedTimecode, VuoMidiTimecode),
//		VuoOutputTrigger(receivedSysEx, VuoMidiSysEx)
)
{
	(*context)->receivedNote = receivedNote;
	(*context)->receivedController = receivedController;
	(*context)->receivedPitchBend = receivedPitchBend;
	VuoMidiIn_enableTriggers((*context)->midiManager, receivedNoteWrapper, receivedControllerWrapper, receivedPitchBendWrapper, *context);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoMidiInputDevice) device,
		VuoOutputTrigger(receivedNote, VuoMidiNote),
		VuoOutputTrigger(receivedController, VuoMidiController),
		VuoOutputTrigger(receivedPitchBend, VuoMidiPitchBend)
)
{
	if (! VuoMidiInputDevice_areEqual(device, (*context)->device))
	{
		VuoMidiIn_disableTriggers((*context)->midiManager);
		updateDevice(*context, device);
		(*context)->receivedNote = receivedNote;
		(*context)->receivedController = receivedController;
		(*context)->receivedPitchBend = receivedPitchBend;
		VuoMidiIn_enableTriggers((*context)->midiManager, receivedNoteWrapper, receivedControllerWrapper, receivedPitchBendWrapper, *context);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoMidiIn_disableTriggers((*context)->midiManager);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoMidiInputDevice_release((*context)->device);
	VuoRelease((*context)->midiManager);
}
