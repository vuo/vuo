/**
 * @file
 * vuo.select.in.boolean.event2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Select Event Input (Boolean)",
					 "keywords" : [ "switch", "multiplexer", "if then else statement", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door" ],
					 "version" : "3.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent({"eventBlocking":"wall","data":"which"}) whichEvent,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) falseOption,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) trueOption,
		VuoOutputEvent() out
)
{
	if (which == false)
		*out = falseOption;
	else
		*out = trueOption;
}
