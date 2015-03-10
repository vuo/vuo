/**
 * @file
 * vuo.select.out node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Output",
					 "keywords" : [ "switch", "demultiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall",
						"boolean", "0", "1", "true", "false" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent(VuoPortEventBlocking_Wall,which) whichEvent,
		VuoInputData(VuoGenericType1) in,
		VuoInputEvent(VuoPortEventBlocking_Door,in) inEvent,
		VuoOutputData(VuoGenericType1) falseOption,
		VuoOutputEvent(falseOption) falseOptionEvent,
		VuoOutputData(VuoGenericType1) trueOption,
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
