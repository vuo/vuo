/**
 * @file
 * vuo.select.out.event node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Event Output (8)",
					 "keywords" : [ "switch", "demultiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":8}) which,
		VuoInputEvent({"eventBlocking":"door","data":"which"}) whichEvent,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) in,
		VuoOutputEvent() option1,
		VuoOutputEvent() option2,
		VuoOutputEvent() option3,
		VuoOutputEvent() option4,
		VuoOutputEvent() option5,
		VuoOutputEvent() option6,
		VuoOutputEvent() option7,
		VuoOutputEvent() option8
)
{
	if (which <= 1)
		*option1 = true;
	else if (which == 2)
		*option2 = true;
	else if (which == 3)
		*option3 = true;
	else if (which == 4)
		*option4 = true;
	else if (which == 5)
		*option5 = true;
	else if (which == 6)
		*option6 = true;
	else if (which == 7)
		*option7 = true;
	else
		*option8 = true;
}
