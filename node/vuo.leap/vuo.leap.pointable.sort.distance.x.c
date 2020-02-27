/**
 * @file
 * vuo.leap.pointable.sort.distance.x node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapPointable.h"
#include "VuoSort.h"

VuoModuleMetadata({
					  "title" : "Sort Pointables by X Distance",
					  "keywords" : [ "organize", "order", "nearest", "point" ],
					  "version" : "1.0.1",
					  "dependencies" : [
						  "VuoSort"
					  ],
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}}) target,
		VuoOutputData(VuoList_VuoLeapPointable) sortedPointables
)
{
	VuoLeapPointable *pointablesData = VuoListGetData_VuoLeapPointable(pointables);
	unsigned long count = VuoListGetCount_VuoLeapPointable(pointables);

	VuoIndexedFloat *distances = (VuoIndexedFloat *)malloc(count * sizeof(VuoIndexedFloat));
	for (unsigned long i = 0; i < count; ++i)
		distances[i] = (VuoIndexedFloat){i, fabs(pointablesData[i].tipPosition.x - target.x)};

	*sortedPointables = VuoListCopy_VuoLeapPointable(pointables);
	VuoLeapPointable *sortedPointablesData = VuoListGetData_VuoLeapPointable(*sortedPointables);

	VuoSort_sortArrayByOtherArray(sortedPointablesData, count, sizeof(VuoLeapPointable), distances);

	free(distances);
}
