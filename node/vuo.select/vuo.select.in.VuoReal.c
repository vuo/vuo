/**
 * @file
 * vuo.select.in.VuoReal node implementation.
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
						If `which` is <i>false</i>, it selects `falseOption`. If `which` is <i>true</i>, it selects `trueOption`.</p> \
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
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent(VuoPortEventBlocking_Wall,which) whichEvent,
		VuoInputData(VuoReal, {"default":0.0}) falseOption,
		VuoInputEvent(VuoPortEventBlocking_Door,falseOption) falseOptionEvent,
		VuoInputData(VuoReal, {"default":0.0}) trueOption,
		VuoInputEvent(VuoPortEventBlocking_Door,trueOption) trueOptionEvent,
		VuoOutputData(VuoReal) out,
		VuoOutputEvent(out) outEvent
)
{
	if (which == false)
	{
		*out = falseOption;
		*outEvent = falseOptionEvent;
	}
	else
	{
		*out = trueOption;
		*outEvent = trueOptionEvent;
	}
}
