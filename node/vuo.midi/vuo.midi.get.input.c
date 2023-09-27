/**
 * @file
 * vuo.midi.get.input node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMidiInputDevice.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "Get MIDI Input Values",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoMidi",
					 ],
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoMidiInputDevice) device,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoText) name
)
{
	VuoMidiInputDevice realizedDevice;
	if (VuoMidiInputDevice_realize(device, &realizedDevice))
	{
		*id = realizedDevice.id;
		*name = realizedDevice.name;
	}
	else
	{
		*id = device.id;
		*name = device.name;
	}
}
