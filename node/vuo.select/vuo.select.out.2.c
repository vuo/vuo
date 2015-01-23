/**
 * @file
 * vuo.select.out.2 node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Output",
					 "keywords" : [ "switch", "demultiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":2}) which,
		VuoInputEvent(VuoPortEventBlocking_Wall,which) whichEvent,
		VuoInputData(VuoGenericType1) in,
		VuoInputEvent(VuoPortEventBlocking_Door,in) inEvent,
		VuoOutputData(VuoGenericType1) option1,
		VuoOutputEvent(option1) option1Event,
		VuoOutputData(VuoGenericType1) option2,
		VuoOutputEvent(option2) option2Event
)
{
	if (which <= 1)
	{
		*option1 = in;
		*option1Event = true;
	}
	else
	{
		*option2 = in;
		*option2Event = true;
	}
}
