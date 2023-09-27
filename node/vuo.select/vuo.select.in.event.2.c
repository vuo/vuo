/**
 * @file
 * vuo.select.in.event node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Select Event Input (2)",
					 "keywords" : [ "switch", "multiplexer", "if then else statement", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door" ],
					 "version" : "2.0.0",
					 "node": {
						  "isDeprecated" : true,
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":2}) which,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option1,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) option2,
		VuoOutputEvent() out
)
{
	if (which <= 1)
		*out = option1;
	else
		*out = option2;
}
