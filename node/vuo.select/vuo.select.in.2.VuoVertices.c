/**
 * @file
 * vuo.select.in.sceneVertices node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Input",
					 "description" :
						"<p>Routes the selected input port's data to the output port.</p> \
						<p>This node is useful for choosing between different data. \
						The `option` inputs can come from different data sources, and the `which` input port	\
						can pick one of them to use.</p> \
						<p>The `which` port selects the `option` input port that will be routed to the output port. \
						If `which` is 1 (or less), it selects `option1`. If `which` is 2 (or more), it selects `option2`.</p> \
						<p>Events that come in through the currently selected `option` port or the refresh port are \
						passed on through the output port. Any other events are blocked.</p> \
						<p>There are two different classes of Select nodes for input and output. \
						The first uses the values 1 and 2 to control how the node functions, while \
						the second uses Boolean true or false values. The first set uses a class name \
						beginning with `vuo.select.in.2` or `vuo.select.out.2` while the second set \
						uses a class name beginning with `vuo.select.in` or `vuo.select.out`.</p>",
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
		VuoInputData(VuoVertices) option1,
		VuoInputEvent(VuoPortEventBlocking_Door,option1) option1Event,
		VuoInputData(VuoVertices) option2,
		VuoInputEvent(VuoPortEventBlocking_Door,option2) option2Event,
		VuoOutputData(VuoVertices) out,
		VuoOutputEvent(out) outEvent
)
{
	if (which <= 1)
	{
		*out = option1;
		*outEvent = option1Event;
	}
	else
	{
		*out = option2;
		*outEvent = option2Event;
	}
}
