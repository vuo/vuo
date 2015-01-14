/**
 * @file
 * vuo.midi.filter.note node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiNote.h"

VuoModuleMetadata({
					 "title" : "Filter MIDI Note",
					 "keywords" : [ "pitch", "tone", "synthesizer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoMidiNote) note,
		VuoInputEvent(VuoPortEventBlocking_Door, note) noteEvent,

		VuoInputData(VuoBoolean, {"default":true}) includeNoteOn,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeNoteOn) includeNoteOnEvent,

		VuoInputData(VuoBoolean, {"default":false}) includeNoteOff,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeNoteOff) includeNoteOffEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":16}) channelMin,
		VuoInputEvent(VuoPortEventBlocking_Wall, channelMin) channelMinEvent,
		VuoInputData(VuoInteger, {"default":16, "suggestedMin":1, "suggestedMax":16}) channelMax,
		VuoInputEvent(VuoPortEventBlocking_Wall, channelMax) channelMaxEvent,

		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":127}) velocityMin,
		VuoInputEvent(VuoPortEventBlocking_Wall, velocityMin) velocityMinEvent,
		VuoInputData(VuoInteger, {"default":127, "suggestedMin":0, "suggestedMax":127}) velocityMax,
		VuoInputEvent(VuoPortEventBlocking_Wall, velocityMax) velocityMaxEvent,

		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":127}) noteNumberMin,
		VuoInputEvent(VuoPortEventBlocking_Wall, noteNumberMin) noteNumberMinEvent,
		VuoInputData(VuoInteger, {"default":127, "suggestedMin":0, "suggestedMax":127}) noteNumberMax,
		VuoInputEvent(VuoPortEventBlocking_Wall, noteNumberMax) noteNumberMaxEvent,

		VuoOutputData(VuoMidiNote) filteredNote,
		VuoOutputEvent(filteredNote) filteredNoteEvent
)
{
	if (note.isNoteOn && !includeNoteOn)
		return;
	if (!note.isNoteOn && !includeNoteOff)
		return;

	if (note.channel < channelMin || note.channel > channelMax)
		return;

	if (note.velocity < velocityMin || note.velocity > velocityMax)
		return;

	if (note.noteNumber < noteNumberMin || note.noteNumber > noteNumberMax)
		return;

	*filteredNote = note;
	*filteredNoteEvent = true;
}
