/**
 * @file
 * vuo.select.in.2.event node implementation.
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
						If `which` is 1 (or less), it selects `option1`. If `which` is 2 (or more), it selects `option2`.</p> \
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
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":2}) which,
		VuoInputEvent(VuoPortEventBlocking_Wall,which) whichEvent,
		VuoInputEvent(VuoPortEventBlocking_Door,) option1,
		VuoInputEvent(VuoPortEventBlocking_Door,) option2,
		VuoOutputEvent() out
)
{
	if (which <= 1)
		*out = option1;
	else
		*out = option2;
}
