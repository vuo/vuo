/**
 * @file
 * vuo.midi.make.input.name node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiInputDevice.h"

VuoModuleMetadata({
					 "title" : "Specify MIDI Input by Name",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) name,
		VuoOutputData(VuoMidiInputDevice) device
)
{
	*device = VuoMidiInputDevice_make(-1, name);
}
