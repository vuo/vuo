/**
 * @file
 * vuo.midi.track.note.poly node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiNote.h"
#include "VuoList_VuoMidiNote.h"

VuoModuleMetadata({
					 "title" : "Track Notes",
					 "keywords" : [ "pitches", "tones", "synthesizer", "music", "instrument",
						 "multiple", "polyphonic", "chord", "range", "octave", "filter", "follower" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "ShowPianoRoll.vuo" ],
					 },
					 "dependencies" : [
						 "VuoList_VuoMidiNote"
					 ]
				 });

VuoList_VuoMidiNote nodeInstanceInit()
{
	return VuoListCreate_VuoMidiNote();
}

void nodeInstanceEvent
(
		VuoInputData(VuoMidiNote) note,
		VuoInputEvent({"eventBlocking":"door","data":"note"}) noteEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":16}) channel,
		VuoInputEvent({"eventBlocking":"wall", "data":"channel"}) channelEvent,

		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":127}) noteNumberMin,
		VuoInputEvent({"eventBlocking":"wall", "data":"noteNumberMin"}) noteNumberMinEvent,
		VuoInputData(VuoInteger, {"default":127, "suggestedMin":0, "suggestedMax":127}) noteNumberMax,
		VuoInputEvent({"eventBlocking":"wall", "data":"noteNumberMax"}) noteNumberMaxEvent,

		VuoInputEvent() reset,

		VuoOutputData(VuoList_VuoMidiNote) notes,
		VuoOutputEvent({"data":"notes"}) notesEvent,

		VuoOutputData(VuoList_VuoInteger) noteNumbers,
		VuoOutputEvent({"data":"noteNumbers"}) noteNumbersEvent,
		VuoOutputData(VuoList_VuoInteger) velocities,
		VuoOutputEvent({"data":"velocities"}) velocitiesEvent,

		VuoInstanceData(VuoList_VuoMidiNote) activeNotes
)
{
	bool updated = false;

	if (reset)
	{
		VuoListRemoveAll_VuoMidiNote(*activeNotes);
		updated = true;
	}

	if (noteEvent && note.channel == channel && note.noteNumber >= noteNumberMin && note.noteNumber <= noteNumberMax)
	{
		unsigned long activeCount = VuoListGetCount_VuoMidiNote(*activeNotes);
		unsigned long activeNoteIndex = 0;
		if (activeCount)
			for (unsigned long i = 1; i <= activeCount; ++i)
			{
				VuoMidiNote activeNote = VuoListGetValue_VuoMidiNote(*activeNotes, i);
				if (note.noteNumber == activeNote.noteNumber)
					activeNoteIndex = i;
			}

		// If the note is active, and we just got a note off, make it inactive.
		if (activeNoteIndex && !note.isNoteOn)
		{
			VuoListRemoveValue_VuoMidiNote(*activeNotes, activeNoteIndex);
			updated = true;
		}

		// If the note is inactive, and we just got a note on, make it active.
		if (activeNoteIndex == 0 && note.isNoteOn)
		{
			VuoListAppendValue_VuoMidiNote(*activeNotes, note);
			updated = true;
		}
	}

	if (updated)
	{
		*notes = *activeNotes;

		/// @todo Once we implement iteration (https://b33p.net/kosada/node/8892), remove these 2 ports and replace connections with an iterated `Get MIDI Note Values`.
		*noteNumbers = VuoListCreate_VuoInteger();
		*velocities = VuoListCreate_VuoInteger();
		unsigned long activeCount = VuoListGetCount_VuoMidiNote(*activeNotes);
		if (activeCount)
			for (unsigned long i = 1; i <= activeCount; ++i)
			{
				VuoMidiNote activeNote = VuoListGetValue_VuoMidiNote(*activeNotes, i);
				VuoListAppendValue_VuoInteger(*noteNumbers, activeNote.noteNumber);
				VuoListAppendValue_VuoInteger(*velocities,  activeNote.velocity);
			}

		*notesEvent = *noteNumbersEvent = *velocitiesEvent = true;
	}
}

void nodeInstanceFini(
		VuoInstanceData(VuoList_VuoMidiNote) activeNotes
)
{
}
