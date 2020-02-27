/**
 * @file
 * vuo.midi.get.output node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiOutputDevice.h"
#include "VuoMidi.h"

VuoModuleMetadata({
					 "title" : "Get MIDI Output Values",
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
		VuoInputData(VuoMidiOutputDevice) device,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoText) name
)
{
	VuoMidiOutputDevice realizedDevice;
	if (VuoMidiOutputDevice_realize(device, &realizedDevice))
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
