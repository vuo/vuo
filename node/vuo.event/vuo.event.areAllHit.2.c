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
						  "exampleCompositions" : [ "RotateInSequence.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) input1,
		VuoInputEvent({"eventBlocking":"door","hasPortAction":false}) input2,
		VuoOutputData(VuoBoolean) allHit,
		VuoOutputEvent({"data":"allHit"}) allHitEvent
)
{
	*allHit = input1 && input2;
	*allHitEvent = true;
}
