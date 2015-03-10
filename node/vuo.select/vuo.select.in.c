/**
 * @file
 * vuo.select.in node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Input",
					 "keywords" : [ "switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door",
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
		VuoInputData(VuoGenericType1) falseOption,
		VuoInputEvent(VuoPortEventBlocking_Door,falseOption) falseOptionEvent,
		VuoInputData(VuoGenericType1) trueOption,
		VuoInputEvent(VuoPortEventBlocking_Door,trueOption) trueOptionEvent,
		VuoOutputData(VuoGenericType1) out,
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
