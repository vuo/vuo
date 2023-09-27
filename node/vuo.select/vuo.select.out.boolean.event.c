/**
 * @file
 * vuo.select.out.boolean.event node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Select Event Output (Boolean)",
					 "keywords" : [ "switch", "demultiplexer", "if then else statement", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent({"eventBlocking":"door","data":"which"}) whichEvent,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) in,
		VuoOutputEvent() falseOption,
		VuoOutputEvent() trueOption
)
{
	if (which == false)
		*falseOption = true;
	else
		*trueOption = true;
}
