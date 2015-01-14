/**
 * @file
 * vuo.leap.filter.hand.id node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Filter Hands by ID",
					  "description" :
						  "<p>Finds all hands in the list that have a certain ID.</p> \
						  <ul> \
						  <li>`hands` — The list of hands to search in.</li> \
						  <li>`id` — The ID to search for. Each hand has a unique ID that can be used to track the hand \
						  across consecutive frames in which the hand is detected. If the hand is lost and re-detected, \
						  its ID changes.</li> \
						  <li>`filteredHands` — All items from `hands` that have ID `id`.</li> \
						  </ul> \
						  ",
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
