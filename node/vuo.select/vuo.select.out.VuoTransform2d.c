/**
 * @file
 * vuo.select.out.VuoTransform2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Output",
					 "description" :
						"<p>Routes the input data to the selected output port.</p> \
						<p>This node is useful for activating different parts of a composition at different times. \
						It can block events to part of the composition unless certain conditions are met.</p> \
						<p>The `which` port selects the `option` output port to which the data will be routed. \
						If `which` is <i>false</i>, it selects `falseOption`. If `which` is <i>true</i>, it selects `trueOption`.</p> \
						<p>Events that come in through the `in` port or the refresh port are passed on through the \
						selected output port.</p> \
						<p>There are two different classes of Select nodes for input and output. \
						The first uses the values 1 and 2 to control how the node functions, while \
						the second uses Boolean true or false values. The first set uses a class name \
						beginning with `vuo.select.in.2` or `vuo.select.out.2` while the second set \
						uses a class name beginning with `vuo.select.in` or `vuo.select.out`.</p>",
					 "keywords" : [ "switch", "demultiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent(VuoPortEventBlocking_Wall,which) whichEvent,
		VuoInputData(VuoTransform2d) in,
		VuoInputEvent(VuoPortEventBlocking_Door,in) inEvent,
		VuoOutputData(VuoTransform2d) falseOption,
		VuoOutputEvent(falseOption) falseOptionEvent,
		VuoOutputData(VuoTransform2d) trueOption,
		VuoOutputEvent(trueOption) trueOptionEvent
)
{
	if (which == false)
	{
		*falseOption = in;
		*falseOptionEvent = true;
	}
	else
	{
		*trueOption = in;
		*trueOptionEvent = true;
	}
}
