/**
 * @file
 * vuo.event.areAllHit.2 node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Are All Hit",
					 "keywords" : [ "boolean" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputEvent({"eventBlocking":"none","hasPortAction":false}) input1,
		VuoInputEvent({"eventBlocking":"none","hasPortAction":false}) input2,
		VuoOutputData(VuoBoolean) allHit,
		VuoOutputEvent({"data":"allHit"}) allHitEvent
)
{
	*allHit = input1 && input2;
	*allHitEvent = true;
}
