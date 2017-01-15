/**
 * @file
 * vuo.midi.make.pitchBend node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiPitchBend.h"

VuoModuleMetadata({
					 "title" : "Make Pitch Bend",
					 "keywords" : [ "wheel", "semitone", "wholetone", "tone", "frequency", "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":16}) channel,
		VuoInputData(VuoInteger, {"default":8192,"suggestedMin":0,"suggestedMax":16383}) value,
		VuoOutputData(VuoMidiPitchBend) pitchBend
)
{
	*pitchBend = VuoMidiPitchBend_make(channel, value);
}
