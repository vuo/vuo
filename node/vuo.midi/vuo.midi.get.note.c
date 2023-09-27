/**
 * @file
 * vuo.midi.get.note node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMidiNote.h"

VuoModuleMetadata({
					 "title" : "Get Note Values",
					 "keywords" : [ "pitch", "tone", "synthesizer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "AnimateForMidiNote.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoMidiNote) note,
		VuoOutputData(VuoInteger) channel,
		VuoOutputData(VuoBoolean) isNoteOn,
		VuoOutputData(VuoInteger) velocity,
		VuoOutputData(VuoInteger) noteNumber
)
{
	*channel = note.channel;
	*isNoteOn = note.isNoteOn;
	*velocity = note.velocity;
	*noteNumber = note.noteNumber;
}
