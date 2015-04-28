/**
 * @file
 * vuo.midi.convert.note.frequency node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiNote.h"

VuoModuleMetadata({
					 "title" : "Convert Note Number to Frequency",
					 "keywords" : [ "pitch", "synthesizer" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":69, "suggestedMin":0, "suggestedMax":127}) noteNumber,
		VuoOutputData(VuoReal) frequency
)
{
	// https://en.wikipedia.org/wiki/MIDI_Tuning_Standard
	*frequency = pow(2., ((noteNumber-69)/12.) ) * 440.;
}
