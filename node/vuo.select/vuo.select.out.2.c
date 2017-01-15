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
					 "title" : "Select Output (2)",
					 "keywords" : [ "switch", "demultiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "RotateOneSquareAtATime.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":2}) which,
		VuoInputEvent({"eventBlocking":"door","data":"which"}) whichEvent,
		VuoInputData(VuoGenericType1) in,
		VuoInputEvent({"eventBlocking":"door","data":"in"}) inEvent,
		VuoOutputData(VuoGenericType1) option1,
		VuoOutputEvent({"data":"option1"}) option1Event,
		VuoOutputData(VuoGenericType1) option2,
		VuoOutputEvent({"data":"option2"}) option2Event
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
