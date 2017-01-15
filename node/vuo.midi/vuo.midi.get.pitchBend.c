/**
 * @file
 * vuo.midi.get.pitchBend node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiPitchBend.h"

VuoModuleMetadata({
					 "title" : "Get Pitch Bend Values",
					 "keywords" : [ "wheel", "semitone", "wholetone", "tone", "frequency", "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoMidiPitchBend) pitchBend,
		VuoOutputData(VuoInteger) channel,
		VuoOutputData(VuoInteger) value
)
{
	*channel = pitchBend.channel;
	*value = pitchBend.value;
}
