/**
 * @file
 * vuo.midi.get.output node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiOutputDevice.h"

VuoModuleMetadata({
					 "title" : "Get MIDI Output Values",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
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
	*id = device.id;
	*name = device.name;
}
