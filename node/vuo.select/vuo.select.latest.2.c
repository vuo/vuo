/**
 * @file
 * vuo.select.latest.2 node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Latest",
					 "keywords" : [ "coalesce", "join", "combine", "recent", "current",
						"switch", "multiplexer", "if", "else", "case", "route", "condition", "control flow" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoGenericType1) option1,
		VuoInputEvent(VuoPortEventBlocking_None,option1) option1Event,
		VuoInputData(VuoGenericType1) option2,
		VuoInputEvent(VuoPortEventBlocking_None,option2) option2Event,
		VuoOutputData(VuoGenericType1) latest
)
{
	if (option2Event && ! option1Event)
		*latest = option2;
	else
		*latest = option1;
}
