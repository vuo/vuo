/**
 * @file
 * vuo.midi.get.note node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get MIDI Note Values",
					 "description" :
						"<p>Gives the components of a MIDI note message.</p> \
						<p><ul> \
						<li>`channel` — The channel, ranging from 1 to 16. Each channel has its own stream of MIDI notes and controller values. A channel often represents one musical instrument. \
						<li>`isNoteOn` — If <i>true</i>, creates a Note On event, representing that a note is depressed (started). \
						If <i>false</i>, creates a Note Off event, representing that a note is released (ended).</li> \
						<li>`velocity` — This often represents the force with which the note is played, ranging from 0 to 127.</li> \
						<li>`noteNumber` — This often represents the pitch of the note, ranging from 0 to 127. Middle C (C4) is 60.</li> \
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
