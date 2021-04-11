/**
 * @file
 * vuo.leap.hand.sort.distance node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapHand.h"
#include "VuoList_VuoLeapHand.h"
#include "VuoSort.h"

VuoModuleMetadata({
					  "title" : "Sort Hands by Distance",
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
		VuoInputData(VuoList_VuoLeapHand) Hands,
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}}) target,
		VuoOutputData(VuoList_VuoLeapHand) sortedHands
)
{
	VuoLeapHand *handsData = VuoListGetData_VuoLeapHand(Hands);
	unsigned long count = VuoListGetCount_VuoLeapHand(Hands);

	VuoIndexedFloat *distances = (VuoIndexedFloat *)malloc(count * sizeof(VuoIndexedFloat));
	for (unsigned long i = 0; i < count; ++i)
		distances[i] = (VuoIndexedFloat){i, VuoPoint3d_squaredMagnitude(VuoPoint3d_subtract(handsData[i].palmPosition, target))};

	*sortedHands = VuoListCopy_VuoLeapHand(Hands);
	VuoLeapHand *sortedHandsData = VuoListGetData_VuoLeapHand(*sortedHands);

	VuoSort_sortArrayByOtherArray(sortedHandsData, count, sizeof(VuoLeapHand), distances);

	free(distances);
}
