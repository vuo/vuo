/**
 * @file
 * vuo.midi.make.device.id node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make MIDI Device from ID",
					 "description" :
						"<p>Finds a MIDI device that is connected to the computer running the composition.</p> \
						<p>This node is useful when you know the ID of the MIDI device that you want. For example, you can \
						use it in place of `Make MIDI Device from Name` if there are multiple devices with the same name \
						and you know the ID of the one you want. If you don't know the ID, then you can instead use \
						`Make MIDI Device from Name` or `List MIDI Devices`.</p> \
						<p><ul> \
						<li>`id` — The MIDI device's ID number, ranging from 0 on up. Each device has a unique ID, \
						which is assigned by the operating system. If all devices were plugged in after the computer started up, \
						then ID 0 is usually the first device plugged in, ID 1 is usually the second device plugged in, etc.</li> \
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
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0}) id,
		VuoInputData(VuoBoolean, {"default":"true"}) isInput,
		VuoOutputData(VuoMidiDevice) device
)
{
	*device = VuoMidiDevice_make(id, VuoText_make(""), isInput);
}
