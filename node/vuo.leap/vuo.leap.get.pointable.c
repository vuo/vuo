/**
 * @file
 * vuo.leap.get.pointable node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoLeapPointableType.h"

VuoModuleMetadata({
					  "title" : "Get Pointable Values",
					  "keywords" : [ "controller", "motion", "finger", "tool" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "DisplayLeapHand.vuo", "HighlightExtendedFingers.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoLeapPointable) pointable,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoLeapPointableType) type,
		VuoOutputData(VuoTransform) transform,
		VuoOutputData(VuoPoint3d) tipVelocity,
		VuoOutputData(VuoLeapTouchZone) touchZone,
		VuoOutputData(VuoReal) touchDistance,
		VuoOutputData(VuoBoolean) isExtended,
		VuoOutputData(VuoReal) timeVisible
)
{
	*id = pointable.id;
	*type = pointable.type;

	VuoPoint4d quaternion = VuoTransform_quaternionFromVectors(VuoPoint3d_make(0,1,0), pointable.direction);
	VuoPoint3d scale = VuoPoint3d_make(pointable.width, pointable.length, pointable.width);
	*transform = VuoTransform_makeQuaternion(pointable.tipPosition, quaternion, scale);

	*tipVelocity = pointable.tipVelocity;
	*touchZone = pointable.touchZone;
	*touchDistance = pointable.touchDistance;
	*isExtended = pointable.isExtended;
	*timeVisible = pointable.timeVisible;
}
