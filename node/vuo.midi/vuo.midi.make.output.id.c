/**
 * @file
 * vuo.midi.make.output.id node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiOutputDevice.h"

VuoModuleMetadata({
					 "title" : "Make MIDI Output from ID",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"name":"ID"}) id,
		VuoOutputData(VuoMidiOutputDevice) device
)
{
	*device = VuoMidiOutputDevice_make(id, VuoText_make(""));
}
