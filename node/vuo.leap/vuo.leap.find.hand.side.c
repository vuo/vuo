/**
 * @file
 * vuo.leap.find.hand.side node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLeapHand.h"
#include "VuoHorizontalSide.h"
#include "VuoList_VuoLeapHand.h"

VuoModuleMetadata({
					  "title" : "Find Hands by Side",
					  "keywords" : [ "controller", "motion", "hand", "palm", "handedness", "direction", "southpaw", "sort", "filter", "search" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "ShowHandStatus.vuo" ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapHand) hands,
		VuoInputData(VuoHorizontalSide, {"default":"left"}) side,
		VuoOutputData(VuoList_VuoLeapHand) foundHands
)
{
	*foundHands = VuoListCreate_VuoLeapHand();

	int count = VuoListGetCount_VuoLeapHand(hands);

	for(int i = 1; i < count+1; i++)
	{
		VuoLeapHand hand = VuoListGetValue_VuoLeapHand(hands, i);

		if( hand.isLeftHand == (side == VuoHorizontalSide_Left) )
			VuoListAppendValue_VuoLeapHand(*foundHands, hand);
	}
}
