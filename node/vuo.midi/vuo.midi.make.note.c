/**
 * @file
 * vuo.midi.make.note node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiNote.h"

VuoModuleMetadata({
					 "title" : "Make Note",
					 "keywords" : [ "pitch", "tone", "synthesizer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "SendMidiNotes.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":16}) channel,
		VuoInputData(VuoBoolean, {"default":true}) isNoteOn,
		VuoInputData(VuoInteger, {"default":127,"suggestedMin":0,"suggestedMax":127}) velocity,
		VuoInputData(VuoInteger, {"default":60,"suggestedMin":0,"suggestedMax":127}) noteNumber,
		VuoOutputData(VuoMidiNote) note
)
{
	*note = VuoMidiNote_make(channel,isNoteOn,velocity,noteNumber);
}
