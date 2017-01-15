/**
 * @file
 * vuo.leap.get.hand node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "DisplayLeapHand.vuo", "ShowHandStatus.vuo", "HoldEgg.vuo", "HighlightExtendedFingers.vuo", "TwirlImageWithLeap.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoLeapHand) hand,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoTransform) transform,
		VuoOutputData(VuoPoint3d) palmVelocity,
		VuoOutputData(VuoPoint3d) wristPosition,
		VuoOutputData(VuoReal) sphereRadius,
		VuoOutputData(VuoPoint3d) sphereCenter,
		VuoOutputData(VuoReal) pinchAmount,
		VuoOutputData(VuoReal) grabAmount,
		VuoOutputData(VuoBoolean) isLeftHand,
		VuoOutputData(VuoReal) timeVisible,
		VuoOutputData(VuoReal) confidence,
		VuoOutputData(VuoList_VuoLeapPointable) fingers
)
{
	*id = hand.id;

	VuoPoint3d scale = VuoPoint3d_make(hand.palmWidth, hand.palmWidth, hand.palmWidth);
	*transform     = VuoTransform_makeQuaternion(hand.palmPosition, hand.rotation, scale);

	*palmVelocity = hand.palmVelocity;
	*wristPosition = hand.wristPosition;
	*sphereRadius = hand.sphereRadius;
	*sphereCenter = hand.sphereCenter;
	*pinchAmount = hand.pinchAmount;
	*grabAmount = hand.grabAmount;
	*isLeftHand = hand.isLeftHand;
	*timeVisible = hand.timeVisible;
	*confidence = hand.confidence;

	*fingers = hand.fingers ? hand.fingers : VuoListCreate_VuoLeapPointable();
}
