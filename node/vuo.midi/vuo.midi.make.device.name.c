/**
 * @file
 * vuo.midi.make.device.name node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiDevice.h"

VuoModuleMetadata({
					 "title" : "Make MIDI Device from Name",
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
