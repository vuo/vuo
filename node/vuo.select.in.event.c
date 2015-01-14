/**
 * @file
 * vuo.select.in.event node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Input",
					 "description" :
						"<p>Routes the selected input port's event to the output port.</p> \
						<p>This node is useful for switching between different ways of controlling part of a composition. \
						For example, the `option` inputs could come from two different input devices, and the `which` \
						input port could choose which input device to use.</p> \
						<p>The `which` port selects the `option` input port that will be routed to the output port. \
						If `which` is <i>false</i>, it selects `falseOption`. If `which` is <i>true</i>, it selects `trueOption`.</p> \
						<p>Events that come in through the currently selected `option` port or the refresh port are \
						passed on through the output port. Any other events are blocked.</p>",
					 "keywords" : [ "switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent(VuoPortEventBlocking_Wall,which) whichEvent,
		VuoInputEvent(VuoPortEventBlocking_Door,) falseOption,
		VuoInputEvent(VuoPortEventBlocking_Door,) trueOption,
		VuoOutputEvent() out
)
{
	if (which == false)
		*out = falseOption;
	else
		*out = trueOption;
}
