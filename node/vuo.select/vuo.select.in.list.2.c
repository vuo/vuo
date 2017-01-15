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
					 "title" : "Select Input List (2)",
					 "keywords" : [ "switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "SelectGradient.vuo", "SelectMovie.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":2}) which,
		VuoInputData(VuoList_VuoGenericType1) option1,
		VuoInputEvent({"eventBlocking":"door","data":"option1"}) option1Event,
		VuoInputData(VuoList_VuoGenericType1) option2,
		VuoInputEvent({"eventBlocking":"door","data":"option2"}) option2Event,
		VuoOutputData(VuoList_VuoGenericType1) out,
		VuoOutputEvent({"data":"out"}) outEvent
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
