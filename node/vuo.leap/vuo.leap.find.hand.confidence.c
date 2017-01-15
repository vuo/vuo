/**
 * @file
 * vuo.leap.find.hand.confidence node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapHand.h"
#include "VuoList_VuoLeapHand.h"

VuoModuleMetadata({
					  "title" : "Find Hands by Confidence",
					  "keywords" : [ "controller", "motion", "hand", "palm", "accurate", "accuracy", "correct", "sort", "filter" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapHand) hands,
		VuoInputData(VuoReal, {"default":0.8, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) confidence,
		VuoOutputData(VuoList_VuoLeapHand) foundHands
)
{
	*foundHands = VuoListCreate_VuoLeapHand();

	int count = VuoListGetCount_VuoLeapHand(hands);

	for(int i = 1; i < count+1; i++)
	{
		VuoLeapHand hand = VuoListGetValue_VuoLeapHand(hands, i);

		if(hand.confidence > confidence)
			VuoListAppendValue_VuoLeapHand(*foundHands, hand);
	}
}
