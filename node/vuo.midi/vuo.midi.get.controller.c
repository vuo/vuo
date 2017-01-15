/**
 * @file
 * vuo.midi.get.controller node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiController.h"

VuoModuleMetadata({
					 "title" : "Get Controller Values",
					 "keywords" : [ "CC", "custom controller", "effect", "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoMidiController) controller,
		VuoOutputData(VuoInteger) channel,
		VuoOutputData(VuoInteger) controllerNumber,
		VuoOutputData(VuoInteger) value
)
{
	*channel = controller.channel;
	*controllerNumber = controller.controllerNumber;
	*value = controller.value;
}
