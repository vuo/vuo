/**
 * @file
 * vuo.select.in.boolean node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Input (Boolean)",
					 "keywords" : [ "switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door",
						"boolean", "0", "1", "true", "false" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "SelectGradient.vuo", "SelectMovie.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputData(VuoGenericType1) falseOption,
		VuoInputEvent({"eventBlocking":"door","data":"falseOption"}) falseOptionEvent,
		VuoInputData(VuoGenericType1) trueOption,
		VuoInputEvent({"eventBlocking":"door","data":"trueOption"}) trueOptionEvent,
		VuoOutputData(VuoGenericType1) out,
		VuoOutputEvent({"data":"out"}) outEvent
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
