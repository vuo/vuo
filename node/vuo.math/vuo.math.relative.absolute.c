/**
 * @file
 * vuo.math.relative.absolute node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Convert Relative to Absolute",
					  "keywords" : [ "sum", "plus", "+", "difference", "minus", "-", "arithmetic", "calculate", "offset", "vector", "point", "cumulative", "accumulative" ],
					  "version" : "1.0.0",
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
	*absoluteValues = VuoListCreate_VuoGenericType1();
	VuoGenericType1 cumulativeSum = VuoGenericType1_makeFromJson(NULL); // 0 / (0,0) / (0,0,0) / (0,0,0,0)
	unsigned long listCount = VuoListGetCount_VuoGenericType1(relativeValues);

	for (unsigned long i = 1; i <= listCount; ++i)
	{
		cumulativeSum = VuoGenericType1_add(cumulativeSum,
											  VuoListGetValue_VuoGenericType1(relativeValues, i));

		VuoListAppendValue_VuoGenericType1(*absoluteValues, cumulativeSum);
	}
}
