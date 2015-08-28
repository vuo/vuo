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
					 "version" : "1.0.0",
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
	VuoMidiDevice device;
	VuoMidiIn midiManager;
};

static void updateDevice(struct nodeInstanceData *context, VuoMidiDevice newDevice)
{
	VuoMidiDevice_release(context->device);
	context->device = newDevice;
	VuoMidiDevice_retain(context->device);

	VuoRelease(context->midiManager);
	context->midiManager = VuoMidiIn_make(newDevice);
	VuoRetain(context->midiManager);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoMidiDevice, {"default":{"isInput":true}}) device
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
	VuoMidiIn_enableTriggers((*context)->midiManager, receivedNote, receivedController);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoMidiDevice, {"default":{"isInput":true}}) device,
		VuoOutputTrigger(receivedNote, VuoMidiNote),
		VuoOutputTrigger(receivedController, VuoMidiController)
)
{
	if (! VuoMidiDevice_areEqual(device, (*context)->device))
	{
		VuoMidiIn_disableTriggers((*context)->midiManager);
		updateDevice(*context, device);
		VuoMidiIn_enableTriggers((*context)->midiManager, receivedNote, receivedController);
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
	VuoMidiDevice_release((*context)->device);
	VuoRelease((*context)->midiManager);
}
