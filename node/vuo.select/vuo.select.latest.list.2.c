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
					 "title" : "Select Latest List (2)",
					 "keywords" : [ "coalesce", "join", "combine", "recent", "current",
						"switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "ShowArrowPresses.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) option1,
		VuoInputEvent({"eventBlocking":"none","data":"option1"}) option1Event,
		VuoInputData(VuoList_VuoGenericType1) option2,
		VuoInputEvent({"eventBlocking":"none","data":"option2"}) option2Event,
		VuoOutputData(VuoList_VuoGenericType1) latest
)
{
	if (option2Event && ! option1Event)
		*latest = option2;
	else
		*latest = option1;
}
