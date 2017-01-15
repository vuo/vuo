/**
 * @file
 * vuo.midi.make.controller node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiController.h"

VuoModuleMetadata({
					 "title" : "Make Controller",
					 "keywords" : [ "CC", "custom controller", "effect", "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":16}) channel,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":0,"suggestedMax":127}) controllerNumber,
		VuoInputData(VuoInteger, {"default":60,"suggestedMin":0,"suggestedMax":127}) value,
		VuoOutputData(VuoMidiController) controller
)
{
	*controller = VuoMidiController_make(channel,controllerNumber,value);
}
