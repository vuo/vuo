/**
 * @file
 * vuo.math.average.group node implementation
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "math.h"

VuoModuleMetadata({
	"title" : "Average List Item Groups",
	"keywords": [
		"mix", "combine", "mean", "midpoint", "middle",
		"cycle", "stride"
	],
	"version" : "1.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoReal",
			"compatibleTypes": [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ],
		},
	},
	"node": {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent
(
	VuoInputData(VuoList_VuoGenericType1) list,
	VuoInputData(VuoInteger, {"name":"Items per Group","default":1,"suggestedMin":1}) itemsPerGroup,
	VuoOutputData(VuoList_VuoGenericType1) averageValues
)
{
	unsigned long clampedItemsPerGroup = MAX(1, itemsPerGroup);
	unsigned long listCount = VuoListGetCount_VuoGenericType1(list);
	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);
	*averageValues = VuoListCreate_VuoGenericType1();

	VuoList_VuoGenericType1 tempList = VuoListCreateWithCount_VuoGenericType1(clampedItemsPerGroup, 0);
	VuoRetain(tempList);

	for (unsigned long i = 0; i < listCount; i += clampedItemsPerGroup)
	{
		// Average final partial group, if applicable.
		unsigned long itemsInCurrentGroup = clampedItemsPerGroup;
		if (listCount - i < clampedItemsPerGroup)
		{
			VuoListRemoveAll_VuoGenericType1(tempList);
			itemsInCurrentGroup = listCount - i;
		}

		for (unsigned long j = 0; j < itemsInCurrentGroup; ++j)
			VuoListSetValue_VuoGenericType1(tempList, listData[i + j], j + 1, true);

		VuoListAppendValue_VuoGenericType1(*averageValues, VuoGenericType1_average(tempList));
	}

	VuoRelease(tempList);
}
