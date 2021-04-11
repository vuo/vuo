/**
 * @file
 * vuo.midi.make.output.name node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiOutputDevice.h"

VuoModuleMetadata({
					 "title" : "Specify MIDI Output by Name",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) name,
		VuoOutputData(VuoMidiOutputDevice) device
)
{
	*device = VuoMidiOutputDevice_make(-1, name);
}
