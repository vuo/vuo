/**
 * @file
 * vuo.midi.make.device.name node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make MIDI Device from Name",
					 "description" :
						"<p>Finds a MIDI device that is connected to the computer running the composition.</p> \
						<p><ul> \
						<li>`name` — All or part of the MIDI device's name, as it appears in the Audio MIDI Setup application. \
						If more than one MIDI device matches the given name, the one with the lowest ID is chosen.</li> \
						<li>`isInput` — If <i>true</i>, finds an input device, which can be used for receiving MIDI events. \
						If <i>false</i>, finds an output device, which can be used for sending MIDI events.</li> \
						</ul></p>",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) name,
		VuoInputData(VuoBoolean, {"default":"true"}) isInput,
		VuoOutputData(VuoMidiDevice) device
)
{
	*device = VuoMidiDevice_make(-1, name, isInput);
}
