/**
 * @file
 * vuo.midi.filter.pitchBend node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiPitchBend.h"

VuoModuleMetadata({
					 "title" : "Filter Pitch Bend",
					 "keywords" : [ "wheel", "semitone", "wholetone", "tone", "frequency", "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "ReceiveMidiNotes.vuo" ]
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoMidiPitchBend) pitchBend,
		VuoInputEvent({"eventBlocking":"door","data":"pitchBend"}) pitchBendEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":16}) channel,
		VuoInputEvent({"eventBlocking":"wall", "data":"channel"}) channelEvent,

		VuoOutputData(VuoInteger) value,
		VuoOutputEvent({"data":"value"}) valueEvent
)
{
	if (pitchBend.channel != channel)
		return;

	*value = pitchBend.value;
	*valueEvent = true;
}
