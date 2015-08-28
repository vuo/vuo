/**
 * @file
 * vuo.event.areAllHit.2 node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Are All Hit",
					 "keywords" : [ "boolean" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputEvent(VuoPortEventBlocking_Door,,{"hasPortAction":false}) input1,
		VuoInputEvent(VuoPortEventBlocking_Door,,{"hasPortAction":false}) input2,
		VuoOutputData(VuoBoolean) allHit,
		VuoOutputEvent(allHit) allHitEvent
)
{
	*allHit = input1 && input2;
	*allHitEvent = true;
}
