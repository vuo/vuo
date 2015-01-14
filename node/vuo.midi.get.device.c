/**
 * @file
 * vuo.midi.get.device node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get MIDI Device Values",
					 "description" :
						"<p>Gives information about a MIDI device.</p> \
						<p><ul> \
						<li>`id` — The MIDI device's ID number, ranging from 0 on up. Each device has a unique ID, \
						which is assigned by the operating system. If all devices were plugged in after the computer started up, \
						then ID 0 is usually the first device plugged in, ID 1 is usually the second device plugged in, etc.</li> \
						<li>`name` — The MIDI device's name, as it appears in the Audio MIDI Setup application.</li> \
						<li>`isInput` — If <i>true</i>, this is an input device, which can be used for receiving MIDI events. \
						If <i>false</i>, this is an output device, which can be used for sending MIDI events.</li> \
						</ul></p>",
					 "keywords" : [ "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoMidiDevice) device,
		VuoOutputData(VuoInteger) id,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoBoolean) isInput
)
{
	*id = device.id;
	*name = device.name;
	*isInput = device.isInput;
}
