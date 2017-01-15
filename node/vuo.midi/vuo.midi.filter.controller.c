/**
 * @file
 * vuo.midi.filter.controller node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiController.h"

VuoModuleMetadata({
					 "title" : "Filter Controller",
					 "keywords" : [ "CC", "custom controller", "effect", "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoMidiController) controller,
		VuoInputEvent({"eventBlocking":"door","data":"controller"}) controllerEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":16}) channel,
		VuoInputEvent({"eventBlocking":"wall", "data":"channel"}) channelEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":0, "suggestedMax":127}) controllerNumber,
		VuoInputEvent({"eventBlocking":"wall", "data":"controllerNumber"}) controllerNumberEvent,

		VuoOutputData(VuoInteger) value,
		VuoOutputEvent({"data":"value"}) valueEvent
)
{
	if (controller.channel != channel || controller.controllerNumber != controllerNumber)
		return;

	*value = controller.value;
	*valueEvent = true;
}
