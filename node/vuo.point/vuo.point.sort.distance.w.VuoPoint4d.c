/**
 * @file
 * vuo.point.sort.distance.w node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSort.h"

VuoModuleMetadata({
	"title": "Sort Points by W Distance",
	"keywords": [
		"organize", "order", "reorder", "nearest",
	],
	"version": "1.0.1",
	"dependencies": [
		"VuoSort",
	],
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint4d) list,
		VuoInputData(VuoPoint4d, {"default":{"x":0, "y":0, "z":0, "w":0}}) point,
		VuoOutputData(VuoList_VuoPoint4d) sorted
)
{
	VuoPoint4d *points = VuoListGetData_VuoPoint4d(list);
	unsigned long count = VuoListGetCount_VuoPoint4d(list);

	VuoIndexedFloat *distances = (VuoIndexedFloat *)malloc(count * sizeof(VuoIndexedFloat));
	for (unsigned long i = 0; i < count; ++i)
		distances[i] = (VuoIndexedFloat){i, fabs(points[i].w - point.w)};

	*sorted = VuoListCopy_VuoPoint4d(list);
	VuoPoint4d *sortedPoints = VuoListGetData_VuoPoint4d(*sorted);

	VuoSort_sortArrayByOtherArray(sortedPoints, count, sizeof(VuoPoint4d), distances);

	free(distances);
}
