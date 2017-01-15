/**
 * @file
 * vuo.select.in.event node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Event Input (8)",
					 "keywords" : [ "switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":8}) which,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option1,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option2,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option3,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option4,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option5,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option6,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option7,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option8,
		VuoOutputEvent() out
)
{
	if (which <= 1)
		*out = option1;
	else if (which == 2)
		*out = option2;
	else if (which == 3)
		*out = option3;
	else if (which == 4)
		*out = option4;
	else if (which == 5)
		*out = option5;
	else if (which == 6)
		*out = option6;
	else if (which == 7)
		*out = option7;
	else
		*out = option8;
}
