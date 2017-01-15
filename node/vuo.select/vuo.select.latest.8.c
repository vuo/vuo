/**
 * @file
 * vuo.select.latest node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Latest (8)",
					 "keywords" : [ "coalesce", "join", "combine", "recent", "current",
						"switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "ShowArrowPresses.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoGenericType1) option1,
		VuoInputEvent({"eventBlocking":"none","data":"option1"}) option1Event,
		VuoInputData(VuoGenericType1) option2,
		VuoInputEvent({"eventBlocking":"none","data":"option2"}) option2Event,
		VuoInputData(VuoGenericType1) option3,
		VuoInputEvent({"eventBlocking":"none","data":"option3"}) option3Event,
		VuoInputData(VuoGenericType1) option4,
		VuoInputEvent({"eventBlocking":"none","data":"option4"}) option4Event,
		VuoInputData(VuoGenericType1) option5,
		VuoInputEvent({"eventBlocking":"none","data":"option5"}) option5Event,
		VuoInputData(VuoGenericType1) option6,
		VuoInputEvent({"eventBlocking":"none","data":"option6"}) option6Event,
		VuoInputData(VuoGenericType1) option7,
		VuoInputEvent({"eventBlocking":"none","data":"option7"}) option7Event,
		VuoInputData(VuoGenericType1) option8,
		VuoInputEvent({"eventBlocking":"none","data":"option8"}) option8Event,
		VuoOutputData(VuoGenericType1) latest
)
{
	if (option1Event)
		*latest = option1;
	else if (option2Event)
		*latest = option2;
	else if (option3Event)
		*latest = option3;
	else if (option4Event)
		*latest = option4;
	else if (option5Event)
		*latest = option5;
	else if (option6Event)
		*latest = option6;
	else if (option7Event)
		*latest = option7;
	else if (option8Event)
		*latest = option8;
	else
		*latest = option1;
}
