/**
 * @file
 * vuo.point.sort.distance.z node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSort.h"

VuoModuleMetadata({
					  "title" : "Sort Points by Z Distance",
					  "keywords" : [ "organize", "order", "nearest" ],
					  "version" : "1.0.1",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "dependencies" : [
						  "VuoSort"
					  ],
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoGenericType1, {"default":{"x":0, "y":0, "z":0, "w":0}}) point,
		VuoOutputData(VuoList_VuoGenericType1) sorted
)
{
	VuoGenericType1 *points = VuoListGetData_VuoGenericType1(list);
	unsigned long count = VuoListGetCount_VuoGenericType1(list);

	VuoIndexedFloat *distances = (VuoIndexedFloat *)malloc(count * sizeof(VuoIndexedFloat));
	for (unsigned long i = 0; i < count; ++i)
		distances[i] = (VuoIndexedFloat){i, fabs(points[i].z - point.z)};

	*sorted = VuoListCopy_VuoGenericType1(list);
	VuoGenericType1 *sortedPoints = VuoListGetData_VuoGenericType1(*sorted);

	VuoSort_sortArrayByOtherArray(sortedPoints, count, sizeof(VuoGenericType1), distances);

	free(distances);
}
