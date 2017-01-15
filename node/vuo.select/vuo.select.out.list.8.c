/**
 * @file
 * vuo.select.out node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Output List (8)",
					 "keywords" : [ "switch", "demultiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "RotateOneSquareAtATime.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":8}) which,
		VuoInputEvent({"eventBlocking":"door","data":"which"}) whichEvent,
		VuoInputData(VuoList_VuoGenericType1) in,
		VuoInputEvent({"eventBlocking":"door","data":"in"}) inEvent,
		VuoOutputData(VuoList_VuoGenericType1) option1,
		VuoOutputEvent({"data":"option1"}) option1Event,
		VuoOutputData(VuoList_VuoGenericType1) option2,
		VuoOutputEvent({"data":"option2"}) option2Event,
		VuoOutputData(VuoList_VuoGenericType1) option3,
		VuoOutputEvent({"data":"option3"}) option3Event,
		VuoOutputData(VuoList_VuoGenericType1) option4,
		VuoOutputEvent({"data":"option4"}) option4Event,
		VuoOutputData(VuoList_VuoGenericType1) option5,
		VuoOutputEvent({"data":"option5"}) option5Event,
		VuoOutputData(VuoList_VuoGenericType1) option6,
		VuoOutputEvent({"data":"option6"}) option6Event,
		VuoOutputData(VuoList_VuoGenericType1) option7,
		VuoOutputEvent({"data":"option7"}) option7Event,
		VuoOutputData(VuoList_VuoGenericType1) option8,
		VuoOutputEvent({"data":"option8"}) option8Event
)
{
	if (which <= 1)
	{
		*option1 = in;
		*option1Event = true;
	}
	else if (which == 2)
	{
		*option2 = in;
		*option2Event = true;
	}
	else if (which == 3)
	{
		*option3 = in;
		*option3Event = true;
	}
	else if (which == 4)
	{
		*option4 = in;
		*option4Event = true;
	}
	else if (which == 5)
	{
		*option5 = in;
		*option5Event = true;
	}
	else if (which == 6)
	{
		*option6 = in;
		*option6Event = true;
	}
	else if (which == 7)
	{
		*option7 = in;
		*option7Event = true;
	}
	else
	{
		*option8 = in;
		*option8Event = true;
	}
}
