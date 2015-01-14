/**
 * @file
 * vuo.test.mouse node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <dispatch/dispatch.h>

VuoModuleMetadata({
					 "title" : "Simulate Mouse Drag",
					 "description" : "Simulates a mouse drag between two points. The x distance is always 42, and the y distance is always 1764.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : true
					 }
				 });

static VuoReal getRandomReal(VuoReal min, VuoReal max)
{
	const uint32_t MAX_RANDOM = -1;
	double normalizedRandom = arc4random() / (double) MAX_RANDOM;
	return normalizedRandom * (max - min) + min;
}

void nodeEvent
(
		VuoOutputTrigger(leftPressed,VuoPoint2d),
		VuoOutputTrigger(leftReleased,VuoPoint2d)
)
{
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
					   VuoPoint2d dragStart, dragEnd;
					   dragStart.x = getRandomReal(0, 1000);
					   dragStart.y = getRandomReal(0, 1000);
					   dragEnd.x = dragStart.x + 42;
					   dragEnd.y = dragStart.y + 1764;

					   leftPressed(dragStart);
					   leftReleased(dragEnd);
				   });
}
