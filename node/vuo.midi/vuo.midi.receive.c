/**
 * @file
 * vuo.midi.receive node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "Receive MIDI Events",
					 "keywords" : [ "note", "controller", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "2.0.0",
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
		VuoOutputTrigger(receivedController, VuoMidiController)
//		VuoOutputTrigger(receivedAftertouch, VuoMidiAftertouch),
//		VuoOutputTrigger(receivedCommand, VuoMidiCommand),
//		VuoOutputTrigger(receivedClock, VuoMidiClock),
//		VuoOutputTrigger(receivedTimecode, VuoMidiTimecode),
//		VuoOutputTrigger(receivedSysEx, VuoMidiSysEx)
)
{
	(*context)->receivedNote = receivedNote;
	(*context)->receivedController = receivedController;
	VuoMidiIn_enableTriggers((*context)->midiManager, receivedNoteWrapper, receivedControllerWrapper, *context);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoMidiInputDevice) device,
		VuoOutputTrigger(receivedNote, VuoMidiNote),
		VuoOutputTrigger(receivedController, VuoMidiController)
)
{
	if (! VuoMidiInputDevice_areEqual(device, (*context)->device))
	{
		VuoMidiIn_disableTriggers((*context)->midiManager);
		updateDevice(*context, device);
		(*context)->receivedNote = receivedNote;
		(*context)->receivedController = receivedController;
		VuoMidiIn_enableTriggers((*context)->midiManager, receivedNoteWrapper, receivedControllerWrapper, *context);
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
