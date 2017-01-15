/**
 * @file
 * vuo.leap.get.frame node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapFrame.h"
#include "VuoLeapHand.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapHand.h"
#include "VuoList_VuoLeapPointable.h"

VuoModuleMetadata({
					  "title" : "Get Frame Values",
					  "keywords" : [ "gesture", "controller", "motion", "hand", "palm", "pointable", "finger", "tool" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "DisplayLeapHand.vuo", "HighlightExtendedFingers.vuo", "TwirlImageWithLeap.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoLeapFrame) frame,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoList_VuoLeapHand) hands,
		VuoOutputData(VuoList_VuoLeapPointable) pointables
)
{
	*id = frame.id;
	*hands = frame.hands ? frame.hands : VuoListCreate_VuoLeapHand();
	*pointables = frame.pointables ? frame.pointables : VuoListCreate_VuoLeapPointable();
}
