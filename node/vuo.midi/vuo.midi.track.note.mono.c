/**
 * @file
 * vuo.midi.track.note.mono node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiNote.h"
#include "VuoList_VuoMidiNote.h"
#include "VuoNotePriority.h"

VuoModuleMetadata({
					 "title" : "Track Single Note",
					 "keywords" : [ "pitches", "tones", "synthesizer", "music", "instrument",
						 "priority", "monophonic", "first", "last", "top", "bottom", "lowest", "highest",
						 "range", "octave", "filter", "follower" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "ReceiveMidiNotes.vuo" ]
					 },
					 "dependencies" : [
						 "VuoList_VuoMidiNote",
						 "VuoList_VuoNotePriority"
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

		VuoInputData(VuoNotePriority, {"default":"last"}) notePriority,
		VuoInputEvent({"eventBlocking":"wall", "data":"notePriority"}) notePriorityEvent,

		VuoInputEvent() reset,

		VuoOutputData(VuoInteger) noteNumber,
		VuoOutputEvent({"data":"noteNumber"}) noteNumberEvent,
		VuoOutputData(VuoInteger) velocity,
		VuoOutputEvent({"data":"velocity"}) velocityEvent,

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

		// If we just got a note on, either make it active or retrigger (by removing then adding to the end).
		if (note.isNoteOn)
		{
			if (activeNoteIndex)
				VuoListRemoveValue_VuoMidiNote(*activeNotes, activeNoteIndex);

			VuoListAppendValue_VuoMidiNote(*activeNotes, note);
			updated = true;
		}
	}

	if (updated)
	{
		unsigned long activeCount = VuoListGetCount_VuoMidiNote(*activeNotes);
		if (!activeCount)
		{
			*velocity = 0;
			*velocityEvent = true;
			return;
		}

		// From all the active notes, choose a winner.
		VuoMidiNote newNote;
		if (notePriority == VuoNotePriority_First)
			newNote = VuoListGetValue_VuoMidiNote(*activeNotes, 1);
		else if (notePriority == VuoNotePriority_Last)
			newNote = VuoListGetValue_VuoMidiNote(*activeNotes, activeCount);
		else if (notePriority == VuoNotePriority_Lowest)
		{
			newNote = VuoMidiNote_make(channel, true, 0, 127);
			for (unsigned long i = 1; i <= activeCount; ++i)
			{
				VuoMidiNote candidate = VuoListGetValue_VuoMidiNote(*activeNotes, i);
				if (candidate.noteNumber < newNote.noteNumber)
					newNote = candidate;
			}
		}
		else // if (notePriority == VuoNotePriority_Highest)
		{
			newNote = VuoMidiNote_make(channel, true, 0, 0);
			for (unsigned long i = 1; i <= activeCount; ++i)
			{
				VuoMidiNote candidate = VuoListGetValue_VuoMidiNote(*activeNotes, i);
				if (candidate.noteNumber > newNote.noteNumber)
					newNote = candidate;
			}
		}

		if (*noteNumber != newNote.noteNumber)
		{
			*noteNumber = newNote.noteNumber;
			*noteNumberEvent = true;
		}
		if (*velocity != newNote.velocity)
		{
			*velocity = newNote.velocity;
			*velocityEvent = true;
		}
	}
}

void nodeInstanceFini(
		VuoInstanceData(VuoList_VuoMidiNote) activeNotes
)
{
}
