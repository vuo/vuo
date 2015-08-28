/**
 * @file
 * vuo.select.out.event node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Select Output",
					 "keywords" : [ "switch", "demultiplexer", "if", "else", "case", "route", "condition", "control flow",
						"activate", "deactivate", "enable", "disable", "choose", "pick", "mode", "block", "door", "wall" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) which,
		VuoInputEvent(VuoPortEventBlocking_Wall,which) whichEvent,
		VuoInputEvent(VuoPortEventBlocking_Door,,{"hasPortAction":false}) in,
		VuoOutputEvent() falseOption,
		VuoOutputEvent() trueOption
)
{
	if (which == false)
		*falseOption = true;
	else
		*trueOption = true;
}
