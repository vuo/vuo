/**
 * @file
 * vuo.select.out.VuoLeapPointable node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"

VuoModuleMetadata({
					 "title" : "Select Output",
					 "description" :
						"<p>Routes the input data to the selected output port.</p> \
						<p>This node is useful for activating different parts of a composition at different times. \
						It can block events to part of the composition unless certain conditions are met.</p> \
						<p>The `which` port selects the `option` output port to which the data will be routed. \
						If `which` is <i>false</i>, it selects `falseOption`. If `which` is <i>true</i>, it selects `trueOption`.</p> \
						<p>Events that come in through the `in` port or the refresh port are passed on through the \
						selected output port.</p>",
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
		VuoInputData(VuoLeapPointable) in,
		VuoInputEvent(VuoPortEventBlocking_Door,in) inEvent,
		VuoOutputData(VuoLeapPointable) falseOption,
		VuoOutputEvent(falseOption) falseOptionEvent,
		VuoOutputData(VuoLeapPointable) trueOption,
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
