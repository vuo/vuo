/**
 * @file
 * vuo.math.multiply.list.2 node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Multiply Lists",
					  "keywords" : [ "scale", "product", "times", "*", "•", "×", "x", "arithmetic", "calculate" ],
					  "version" : "1.0.1",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

static const VuoInteger VuoInteger_multiplicativeIdentity = 1;
static const VuoReal    VuoReal_multiplicativeIdentity    = 1;
static const VuoPoint2d VuoPoint2d_multiplicativeIdentity = (VuoPoint2d){1,1};
static const VuoPoint3d VuoPoint3d_multiplicativeIdentity = (VuoPoint3d){1,1,1};
static const VuoPoint4d VuoPoint4d_multiplicativeIdentity = (VuoPoint4d){1,1,1,1};

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1, {"name":"Values 1"}) list1,
		VuoInputData(VuoList_VuoGenericType1, {"name":"Values 2"}) list2,
		VuoOutputData(VuoList_VuoGenericType1, {"name":"Products"}) productList
)
{
	unsigned long list1Count = VuoListGetCount_VuoGenericType1(list1);
	unsigned long list2Count = VuoListGetCount_VuoGenericType1(list2);
	unsigned long maxCount = MAX(list1Count, list2Count);

	VuoGenericType1 *list1Data = VuoListGetData_VuoGenericType1(list1);
	VuoGenericType1 *list2Data = VuoListGetData_VuoGenericType1(list2);

	*productList = VuoListCreateWithCount_VuoGenericType1(maxCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *productListData = VuoListGetData_VuoGenericType1(*productList);
	for (unsigned long i = 1; i <= maxCount; ++i)
	{
		VuoGenericType1 v1;
		if (list1Count)
			v1 = list1Data[VuoListIndexToCArrayIndex(i, list1Count)];
		else
			v1 = VuoGenericType1_multiplicativeIdentity;

		VuoGenericType1 v2;
		if (list2Count)
			v2 = list2Data[VuoListIndexToCArrayIndex(i, list2Count)];
		else
			v2 = VuoGenericType1_multiplicativeIdentity;

		productListData[i - 1] = v1 * v2;
	}
}
