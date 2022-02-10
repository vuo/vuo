/**
 * @file
 * vuo.math.relative.absolute node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Convert Relative to Absolute",
					  "keywords" : [ "sum", "plus", "+", "difference", "minus", "-", "arithmetic", "calculate", "offset", "vector", "point", "cumulative", "accumulative" ],
					  "version" : "1.0.1",
					  "dependencies" : [ ],
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) relativeValues,
		VuoOutputData(VuoList_VuoGenericType1) absoluteValues
)
{
	unsigned long listCount = VuoListGetCount_VuoGenericType1(relativeValues);
	if (listCount == 0)
	{
		*absoluteValues = NULL;
		return;
	}

	VuoGenericType1 cumulativeSum = VuoGenericType1_makeFromJson(NULL); // 0 / (0,0) / (0,0,0) / (0,0,0,0)
	VuoGenericType1 *relativeValuesData = VuoListGetData_VuoGenericType1(relativeValues);
	*absoluteValues = VuoListCreateWithCount_VuoGenericType1(listCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *absoluteValuesData = VuoListGetData_VuoGenericType1(*absoluteValues);
	for (unsigned long i = 0; i < listCount; ++i)
	{
		cumulativeSum += relativeValuesData[i];
		absoluteValuesData[i] = cumulativeSum;
	}
}
