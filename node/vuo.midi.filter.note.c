/**
 * @file
 * vuo.midi.filter.note node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Filter MIDI Note",
					 "description" :
						"<p>Only lets a MIDI note pass through if it matches all conditions.</p> \
						<p><ul> \
						<li>`note` — An event into this port is blocked unless the port's value meets all of the criteria \
						specified by the other input ports.</li> \
						<li>`includeNoteOn`, `includeNoteOff` — If <i>true</i>, accept Note On or Note Off messages. \
						At least one of these ports must be <i>true</i> for this node to accept any MIDI notes. \
						A Note On message represents that a note is depressed (started). A Note Off message represents that a note is released (ended).</li> \
						<li>`channelMin`, `channelMax` — The range of accepted channels. For all possible channels, use 1 to 16. \
						Each channel has its own stream of MIDI notes and controller values. A channel often represents one musical instrument.</li> \
						<li>`velocityMin`, `velocityMax` — The range of accepted velocities. For all possible velocities, use 0 to 127. \
						The velocity often represents the force with which the note is played.</li> \
						<li>`noteNumberMin`, `noteNumberMax` — The range of accepted note numbers. For all possible note numbers, use 0 to 127. \
						The note number often represents the pitch of the note, with Middle C (C4) at 60.</li> \
						</ul></p>",
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
