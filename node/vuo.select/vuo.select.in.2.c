/**
 * @file
 * vuo.select.in.2 node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Input",
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
		VuoInputData(VuoGenericType1) option1,
		VuoInputEvent(VuoPortEventBlocking_Door,option1) option1Event,
		VuoInputData(VuoGenericType1) option2,
		VuoInputEvent(VuoPortEventBlocking_Door,option2) option2Event,
		VuoOutputData(VuoGenericType1) out,
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
