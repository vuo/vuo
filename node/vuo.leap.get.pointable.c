/**
 * @file
 * vuo.leap.get.pointable node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get Pointable Values",
					  "description" :
						  "<p>Gives information about a pointable (finger or tool) detected within a frame from a Leap Motion device.</p> \
						  <ul> \
						  <li>`id` - A unique ID for this pointable. The ID can be used to track the pointable across consecutive \
						  frames in which the pointable is detected. If the pointable is lost and re-detected, its ID changes.</li> \
						  <li>`type` - The type of pointable (finger or tool). Leap Motion considers a pointable to be a tool \
						  if it's thinner, straighter, and longer than Leap Motion's parameters for a finger.</li> \
						  <li>`length` - The length of this pointable, from the hand to the tip (or whatever portion is visible), \
						  in millimeters. If Leap Motion can't determine the length, this outputs 0.</li> \
						  <li>`width` - The average width of this pointable (or whatever portion is visible), in millimeters. \
						  If Leap Motion can't determine the width, this outputs 0.</li> \
						  <li>`direction` - A unit vector pointing in the same direction as this pointable's tip.</li> \
						  <li>`tipPosition` - The position of this pointable's tip, in Vuo coordinates.</li> \
						  <li>`tipVelocity` - The rate of change of `tipPosition`, in millimeters per second.</li> \
						  <ul> \
						  <p>The Vuo coordinates are based on Leap Motion's interaction box, \
						  which is a rectangular box within the Leap Motion device's field of view. \
						  Most most hand motions are expected to take place within the interaction box, \
						  although some may fall outside of it and still be detected by the Leap Motion. \
						  The interaction box's center is at Vuo coordinate (0,0,0). \
						  The interaction box's left edge is at Vuo x-coordinate -1, and its right edge is at Vuo x-coordinate 1. \
						  The Vuo y-coordinates increase from bottom to top of the interaction box. \
						  The Vuo z-coordinates increase from back to front of the interaction box.</p> \
						  ",
					  "keywords" : [ "controller", "motion", "finger", "tool" ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  ],
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoLeapPointable) pointable,
		VuoOutputData(VuoInteger) id,
		VuoOutputData(VuoLeapPointableType) type,
		VuoOutputData(VuoReal) length,
		VuoOutputData(VuoReal) width,
		VuoOutputData(VuoPoint3d) direction,
		VuoOutputData(VuoPoint3d) tipPosition,
		VuoOutputData(VuoPoint3d) tipVelocity
)
{
	*id = pointable.id;
	*type = pointable.type;
	*length = pointable.length;
	*width = pointable.width;
	*direction	= pointable.direction;
	*tipPosition = pointable.tipPosition;
	*tipVelocity = pointable.tipVelocity;
}
