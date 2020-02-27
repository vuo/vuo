/**
 * @file
 * vuo.midi.make.input.id node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiInputDevice.h"

VuoModuleMetadata({
					 "title" : "Specify MIDI Input by ID",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"name":"ID"}) id,
		VuoOutputData(VuoMidiInputDevice) device
)
{
	*device = VuoMidiInputDevice_make(id, VuoText_make(""));
}
