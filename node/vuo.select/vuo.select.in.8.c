/**
 * @file
 * vuo.select.in node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Input (8)",
					 "keywords" : [ "switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "SelectGradient.vuo", "SelectMovie.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":8}) which,
		VuoInputData(VuoGenericType1) option1,
		VuoInputEvent({"eventBlocking":"door","data":"option1"}) option1Event,
		VuoInputData(VuoGenericType1) option2,
		VuoInputEvent({"eventBlocking":"door","data":"option2"}) option2Event,
		VuoInputData(VuoGenericType1) option3,
		VuoInputEvent({"eventBlocking":"door","data":"option3"}) option3Event,
		VuoInputData(VuoGenericType1) option4,
		VuoInputEvent({"eventBlocking":"door","data":"option4"}) option4Event,
		VuoInputData(VuoGenericType1) option5,
		VuoInputEvent({"eventBlocking":"door","data":"option5"}) option5Event,
		VuoInputData(VuoGenericType1) option6,
		VuoInputEvent({"eventBlocking":"door","data":"option6"}) option6Event,
		VuoInputData(VuoGenericType1) option7,
		VuoInputEvent({"eventBlocking":"door","data":"option7"}) option7Event,
		VuoInputData(VuoGenericType1) option8,
		VuoInputEvent({"eventBlocking":"door","data":"option8"}) option8Event,
		VuoOutputData(VuoGenericType1) out,
		VuoOutputEvent({"data":"out"}) outEvent
)
{
	if (which <= 1)
	{
		*out = option1;
		*outEvent = option1Event;
	}
	else if (which == 2)
	{
		*out = option2;
		*outEvent = option2Event;
	}
	else if (which == 3)
	{
		*out = option3;
		*outEvent = option3Event;
	}
	else if (which == 4)
	{
		*out = option4;
		*outEvent = option4Event;
	}
	else if (which == 5)
	{
		*out = option5;
		*outEvent = option5Event;
	}
	else if (which == 6)
	{
		*out = option6;
		*outEvent = option6Event;
	}
	else if (which == 7)
	{
		*out = option7;
		*outEvent = option7Event;
	}
	else
	{
		*out = option8;
		*outEvent = option8Event;
	}
}
