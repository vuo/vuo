/**
 * @file
 * vuo.leap.get.hand node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapHand.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapPointable.h"

VuoModuleMetadata({
					  "title" : "Get Hand Values",
					  "keywords" : [ "controller", "motion", "hand", "palm" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false,
						  "exampleCompositions" : [ "DisplayLeapHand.vuo", "TwirlImageWithLeap.vuo" ]
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
	*pointables = hand.pointables ? hand.pointables : VuoListCreate_VuoLeapPointable();
}
