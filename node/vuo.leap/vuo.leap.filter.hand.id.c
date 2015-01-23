/**
 * @file
 * vuo.leap.filter.hand.id node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapHand.h"
#include "VuoList_VuoLeapHand.h"

VuoModuleMetadata({
					  "title" : "Filter Hands by ID",
					  "keywords" : [ "controller", "motion", "hand", "palm" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapHand) hands,
		VuoInputData(VuoInteger, {"default":0}) id,
		VuoOutputData(VuoList_VuoLeapHand) filteredHands
)
{
	*filteredHands = VuoListCreate_VuoLeapHand();
	int count = VuoListGetCount_VuoLeapHand(hands);

	for(int i = 1; i < count+1; i++)
	{
		VuoLeapHand hand = VuoListGetValueAtIndex_VuoLeapHand(hands, i);

		if( hand.id == id )
			VuoListAppendValue_VuoLeapHand(*filteredHands, hand);
	}

}
