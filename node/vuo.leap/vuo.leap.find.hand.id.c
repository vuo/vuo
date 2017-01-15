/**
 * @file
 * vuo.leap.find.hand.id node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapHand.h"
#include "VuoList_VuoLeapHand.h"

VuoModuleMetadata({
					  "title" : "Find Hands by ID",
					  "keywords" : [ "controller", "motion", "hand", "palm", "filter" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapHand) hands,
		VuoInputData(VuoInteger, {"default":0, "name":"ID"}) id,
		VuoOutputData(VuoList_VuoLeapHand) foundHands
)
{
	*foundHands = VuoListCreate_VuoLeapHand();
	int count = VuoListGetCount_VuoLeapHand(hands);

	for(int i = 1; i < count+1; i++)
	{
		VuoLeapHand hand = VuoListGetValue_VuoLeapHand(hands, i);

		if( hand.id == id )
			VuoListAppendValue_VuoLeapHand(*foundHands, hand);
	}

}
