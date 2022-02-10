/**
 * @file
 * vuo.list.spread.group node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					"title" : "Copy List Item Groups",
					"keywords" : [ "duplicate", "copy", "clone", "double", "repeat", "replicate", "expand", "fill", "stretch",
						"cycle", "stride"
					],
					"version" : "1.0.2",
					"node": {
						"exampleCompositions" : [ "StringAlternatingBeads.vuo" ]
					}
				});

void nodeEvent
(
	VuoInputData(VuoList_VuoGenericType1) list,
	VuoInputData(VuoInteger, {"default":2,"suggestedMin":1}) copies,
	VuoInputData(VuoInteger, {"default":2,"suggestedMin":1,"name":"Items per Group"}) itemsPerGroup,
	VuoOutputData(VuoList_VuoGenericType1) outputList
)
{
	unsigned long inputCount = VuoListGetCount_VuoGenericType1(list);
	unsigned long clampedCopies = MAX(0, copies);
	unsigned long clampedItemsPerGroup = MAX(1, itemsPerGroup);
	unsigned long outputCount = ceilf(inputCount / (float)clampedItemsPerGroup) * clampedCopies * clampedItemsPerGroup;
	if (outputCount == 0)
	{
		*outputList = NULL;
		return;
	}

	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);
	*outputList = VuoListCreateWithCount_VuoGenericType1(outputCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *outputListData = VuoListGetData_VuoGenericType1(*outputList);
	unsigned long m = 0;
	for (unsigned long i = 1; i <= inputCount; i += clampedItemsPerGroup)
		for (unsigned long j = 0; j < clampedCopies; ++j)
			for (unsigned long k = 0; k < clampedItemsPerGroup; ++k)
			{
				outputListData[m] = listData[VuoListIndexToCArrayIndex(i + k, inputCount)];
				VuoGenericType1_retain(outputListData[m++]);
			}
}
