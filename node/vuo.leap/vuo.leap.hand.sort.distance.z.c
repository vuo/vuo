/**
 * @file
 * vuo.leap.hand.sort.distance.z node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapHand.h"
#include "VuoList_VuoLeapHand.h"
#include "VuoSort.h"

VuoModuleMetadata({
					  "title" : "Sort Hands by Z Distance",
					  "keywords" : [ "organize", "order", "nearest", "point" ],
					  "version" : "1.0.1",
					  "dependencies" : [
						  "VuoSort"
					  ],
					  "node": {
						  "exampleCompositions" : [ "DisplayLeapHand.vuo", "HighlightExtendedFingers.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoLeapHand) hands,
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}}) target,
		VuoOutputData(VuoList_VuoLeapHand) sortedHands
)
{
	VuoLeapHand *handsData = VuoListGetData_VuoLeapHand(hands);
	unsigned long count = VuoListGetCount_VuoLeapHand(hands);

	VuoIndexedFloat *distances = (VuoIndexedFloat *)malloc(count * sizeof(VuoIndexedFloat));
	for (unsigned long i = 0; i < count; ++i)
		distances[i] = (VuoIndexedFloat){i, fabs(handsData[i].palmPosition.z - target.z)};

	*sortedHands = VuoListCopy_VuoLeapHand(hands);
	VuoLeapHand *sortedHandsData = VuoListGetData_VuoLeapHand(*sortedHands);

	VuoSort_sortArrayByOtherArray(sortedHandsData, count, sizeof(VuoLeapHand), distances);

	free(distances);
}
