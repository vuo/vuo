/**
 * @file
 * vuo.leap.get.hand node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get Hand Values",
					  "description" :
						  "<p>Gives information about a hand detected within a frame from a Leap Motion device.</p> \
						  <ul> \
						  <li>`id` - A unique ID for this hand. The ID can be used to track the hand across consecutive \
						  frames in which the hand is detected. If the hand is lost and re-detected, its ID changes.</li> \
						  <li>`direction` - A unit vector pointing from `palmPosition` toward the fingers.</li> \
						  <li>`palmNormal` - A unit vector pointing outward from (perpendicular to) the front surface of the palm.</li> \
						  <li>`palmPosition` - The center of the palm, in Vuo coordinates.</li> \
						  <li>`palmVelocity` - The rate of change of `palmPosition`, in millimeters per second.</li> \
						  <li>`sphereRadius` - The radius of a ball that could be held by this hand, in Vuo coordinates.</li> \
						  <li>`sphereCenter` - The center of a ball that could be held by this hand, in Vuo coordinates.</li> \
						  <li>`pointables` - A list of all pointables (fingers and tools) attached to this hand.</li> \
						  </ul> \
						  <p>`sphereRadius` and `sphereCenter` are based on a sphere calculated to fit the curvature \
						  of the palm and finge. It approximates the largest ball that the hand could hold in its current position.</p> \
						  <p>The Vuo coordinates are based on Leap Motion's interaction box, \
						  which is a rectangular box within the Leap Motion device's field of view. \
						  Most most hand motions are expected to take place within the interaction box, \
						  although some may fall outside of it and still be detected by the Leap Motion. \
						  The interaction box's center is at Vuo coordinate (0,0,0). \
						  The interaction box's left edge is at Vuo x-coordinate -1, and its right edge is at Vuo x-coordinate 1. \
						  The Vuo y-coordinates increase from bottom to top of the interaction box. \
						  The Vuo z-coordinates increase from back to front of the interaction box.</p> \
						  ",
					  "keywords" : [ "controller", "motion", "hand", "palm" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoLeapHand) hand,
		VuoOutputData(VuoInteger) id,
		VuoOutputData(VuoPoint3d) direction,
		VuoOutputData(VuoPoint3d) palmNormal,
		VuoOutputData(VuoPoint3d) palmPosition,
		VuoOutputData(VuoPoint3d) palmVelocity,
		VuoOutputData(VuoReal) sphereRadius,
		VuoOutputData(VuoPoint3d) sphereCenter,
		VuoOutputData(VuoList_VuoLeapPointable) pointables
)
{
	*id = hand.id;
	*direction = hand.direction;
	*palmNormal = hand.palmNormal;
	*palmPosition = hand.palmPosition;
	*palmVelocity = hand.palmVelocity;
	*sphereRadius = hand.sphereRadius;
	*sphereCenter = hand.sphereCenter;
	*pointables = hand.pointables;
}
