/**
 * @file
 * vuo.midi.filter.controller node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Filter MIDI Controller",
					 "description" :
						"<p>Only lets a MIDI controller pass through if it matches all conditions.</p> \
						<p><ul> \
						<li>`controller` — An event into this port is blocked unless the port's value meets all of the criteria \
						specified by the other input ports.</li> \
						<li>`channelMin`, `channelMax` — The range of accepted channels. For all possible channels, use 1 to 16. \
						Each channel has its own stream of MIDI notes and controller values. A channel often represents one musical instrument.</li> \
						<li>`controllerNumberMin`, `controllerNumberMax` — The range of accepted controller numbers. For all possible controller numbers, use 0 to 127. \
						The controller number represents the effect parameter (volume, panning, filter cutoff, sustain, etc.).</li> \
						<li>`valueMin`, `valueMax` — The range of accepted controller values. For all possible controller values, use 0 to 127. \
						The controller value represents the amount of the effect parameter. Some effect parameters use the \
						whole range of values. Other effect parameters are either on (0-63) or off (64-127).</li> \
						</ul></p>",
					 "keywords" : [ "CC", "custom controller", "effect", "synthesizer", "sequencer", "music", "instrument" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoMidiController) controller,
		VuoInputEvent(VuoPortEventBlocking_Door, controller) controllerEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":16}) channelMin,
		VuoInputEvent(VuoPortEventBlocking_Wall, channelMin) channelMinEvent,
		VuoInputData(VuoInteger, {"default":16, "suggestedMin":1, "suggestedMax":16}) channelMax,
		VuoInputEvent(VuoPortEventBlocking_Wall, channelMax) channelMaxEvent,

		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":127}) controllerNumberMin,
		VuoInputEvent(VuoPortEventBlocking_Wall, controllerNumberMin) controllerNumberMinEvent,
		VuoInputData(VuoInteger, {"default":127, "suggestedMin":0, "suggestedMax":127}) controllerNumberMax,
		VuoInputEvent(VuoPortEventBlocking_Wall, controllerNumberMax) controllerNumberMaxEvent,

		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":127}) valueMin,
		VuoInputEvent(VuoPortEventBlocking_Wall, valueMin) valueMinEvent,
		VuoInputData(VuoInteger, {"default":127, "suggestedMin":0, "suggestedMax":127}) valueMax,
		VuoInputEvent(VuoPortEventBlocking_Wall, valueMax) valueMaxEvent,

		VuoOutputData(VuoMidiController) filteredController,
		VuoOutputEvent(filteredController) filteredControllerEvent
)
{
	if (controller.channel < channelMin || controller.channel > channelMax)
		return;

	if (controller.controllerNumber < controllerNumberMin || controller.controllerNumber > controllerNumberMax)
		return;

	if (controller.value < valueMin || controller.value > valueMax)
		return;

	*filteredController = controller;
	*filteredControllerEvent = true;
}
