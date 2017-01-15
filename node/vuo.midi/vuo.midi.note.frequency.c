/**
 * @file
 * vuo.midi.note.frequency node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiNote.h"

VuoModuleMetadata({
					 "title" : "Convert Note to Frequency",
					 "keywords" : [ "pitch", "synthesizer", "hertz", "twelvetone", "dodecaphonic", "equal", "temperament", "tempered", "intonation" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "ReceiveMidiNotes.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":69, "suggestedMin":0, "suggestedMax":127}) noteNumber,
		VuoInputData(VuoInteger, {"default":8192, "suggestedMin":0, "suggestedMax":16383}) pitchBend,///< Permitted values: 0 through 16383; center=8192
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":0, "suggestedMax":24}) pitchBendRange,
		VuoOutputData(VuoReal) frequency
)
{
	// https://en.wikipedia.org/wiki/MIDI_Tuning_Standard
	*frequency = pow(2., (noteNumber-69 + (pitchBend/8192. - 1.) * pitchBendRange)/12.) * 440.;
}
