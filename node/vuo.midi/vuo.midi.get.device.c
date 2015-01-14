/**
 * @file
 * vuo.midi.get.device node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiDevice.h"

VuoModuleMetadata({
					 "title" : "Get MIDI Device Values",
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
