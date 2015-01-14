/**
 * @file
 * vuo.midi.receive node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
						 "isInterface" : true
					 }
				 });


VuoMidiIn nodeInstanceInit(
		VuoInputData(VuoMidiDevice, {"default":{"isInput":true}}) device
)
{
	VuoMidiIn mi = VuoMidiIn_make(device);
	VuoRetain(mi);
	return mi;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(VuoMidiIn) context,
		VuoOutputTrigger(receivedNote, VuoMidiNote),
		VuoOutputTrigger(receivedController, VuoMidiController)
//		VuoOutputTrigger(receivedAftertouch, VuoMidiAftertouch),
//		VuoOutputTrigger(receivedCommand, VuoMidiCommand),
//		VuoOutputTrigger(receivedClock, VuoMidiClock),
//		VuoOutputTrigger(receivedTimecode, VuoMidiTimecode),
//		VuoOutputTrigger(receivedSysEx, VuoMidiSysEx)
)
{
	VuoMidiIn_enableTriggers(*context, receivedNote, receivedController);
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoMidiIn) context,
		VuoInputData(VuoMidiDevice, {"default":{"isInput":true}}) device,
		VuoInputEvent(VuoPortEventBlocking_Wall, device) deviceEvent
)
{
	if (deviceEvent)
	{
		VuoRelease(*context);
		*context = VuoMidiIn_make(device);
		VuoRetain(*context);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(VuoMidiIn) context
)
{
	VuoMidiIn_disableTriggers(*context);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoMidiIn) context
)
{
	VuoRelease(*context);
}
