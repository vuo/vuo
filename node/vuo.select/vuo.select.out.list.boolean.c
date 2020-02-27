/**
 * @file
 * vuo.select.out.boolean node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Output List (Boolean)",
					 "keywords" : [ "switch", "demultiplexer", "if then else statement", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall",
						"boolean", "0", "1", "true", "false" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "RotateOneSquareAtATime.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent({"eventBlocking":"door","data":"which"}) whichEvent,
		VuoInputData(VuoList_VuoGenericType1) in,
		VuoInputEvent({"eventBlocking":"door","data":"in"}) inEvent,
		VuoOutputData(VuoList_VuoGenericType1) falseOption,
		VuoOutputEvent({"data":"falseOption"}) falseOptionEvent,
		VuoOutputData(VuoList_VuoGenericType1) trueOption,
		VuoOutputEvent({"data":"trueOption"}) trueOptionEvent
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
