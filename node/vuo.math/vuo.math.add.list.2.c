/**
 * @file
 * vuo.math.add.list.2 node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Add Lists",
					  "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate", "vector", "point", "combine" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list1,
		VuoInputData(VuoList_VuoGenericType1) list2,
		VuoOutputData(VuoList_VuoGenericType1) summedList
)
{
	*summedList = VuoListCreate_VuoGenericType1();
	unsigned long list1Count = VuoListGetCount_VuoGenericType1(list1);
	unsigned long list2Count = VuoListGetCount_VuoGenericType1(list2);
	unsigned long maxCount = MAX(list1Count, list2Count);
	for (unsigned long i = 1; i <= maxCount; ++i)
	{
		VuoGenericType1 sum;
		if (i <= list1Count && i <= list2Count)
			sum = VuoGenericType1_add(
						VuoListGetValue_VuoGenericType1(list1, i),
						VuoListGetValue_VuoGenericType1(list2, i));
		else if (i <= list1Count)
			sum = VuoListGetValue_VuoGenericType1(list1, i);
		else if (i <= list2Count)
			sum = VuoListGetValue_VuoGenericType1(list2, i);

		VuoListAppendValue_VuoGenericType1(*summedList, sum);
	}
}
