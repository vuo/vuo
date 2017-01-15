/**
 * @file
 * vuo.midi.filter.note node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiNote.h"

VuoModuleMetadata({
					 "title" : "Filter Note",
					 "keywords" : [ "pitch", "tone", "synthesizer", "music", "instrument" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "AnimateForMidiNote.vuo" ]
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoMidiNote) note,
		VuoInputEvent({"eventBlocking":"door", "data":"note"}) noteEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":16}) channel,
		VuoInputEvent({"eventBlocking":"wall", "data":"channel"}) channelEvent,

		VuoInputData(VuoInteger, {"default":60, "suggestedMin":0, "suggestedMax":127}) noteNumber,
		VuoInputEvent({"eventBlocking":"wall", "data":"noteNumber"}) noteNumberEvent,

		VuoOutputData(VuoInteger) velocity,
		VuoOutputEvent({"data":"velocity"}) velocityEvent
)
{
	if (note.channel != channel || note.noteNumber != noteNumber)
		return;

	*velocity = note.isNoteOn ? note.velocity : 0;
	*velocityEvent = true;
}
