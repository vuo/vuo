/**
 * @file
 * vuo.midi.make.controller node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make MIDI Controller",
					 "description" :
						"<p>Creates a MIDI controller message that can be sent to a MIDI device.</p> \
						<p>A MIDI controller message represents an effect parameter (volume, panning, filter cutoff, sustain, etc.).</p> \
						<p><ul> \
						<li>`channel` — The channel, ranging from 1 to 16. Each channel has its own stream of MIDI notes and controller values. \
						A channel often represents one musical instrument. \
						<li>`controllerNumber` — The effect parameter, ranging from 0 to 127. \
						<li>`value` — The amount of the effect parameter, ranging from 0 to 127. Some effect parameters use the \
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
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":16}) channel,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":0,"suggestedMax":127}) controllerNumber,
		VuoInputData(VuoInteger, {"default":60,"suggestedMin":0,"suggestedMax":127}) value,
		VuoOutputData(VuoMidiController) controller
)
{
	*controller = VuoMidiController_make(channel,controllerNumber,value);
}
