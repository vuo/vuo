/**
 * @file
 * vuo.list.interleave.group node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Interleave List Item Groups",
	"keywords": [
		"combine", "join", "together", "merge",
		"alternate", "intersperse", "spread", "take turns", "rotate",
		"cycle", "stride",
		"array", "set", "sequence",
		"items", "index", "indices",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoGenericType1) list1,
	VuoInputData(VuoInteger, {"name":"List 1 Items per Group","default":1,"suggestedMin":1}) list1ItemsPerGroup,
	VuoInputData(VuoList_VuoGenericType1) list2,
	VuoInputData(VuoInteger, {"name":"List 2 Items per Group","default":1,"suggestedMin":1}) list2ItemsPerGroup,
	VuoOutputData(VuoList_VuoGenericType1) interleavedList)
{
	unsigned long clampedList1ItemsPerGroup = MAX(1, list1ItemsPerGroup);
	unsigned long clampedList2ItemsPerGroup = MAX(1, list2ItemsPerGroup);

	unsigned long list1Count = VuoListGetCount_VuoGenericType1(list1);
	unsigned long list2Count = VuoListGetCount_VuoGenericType1(list2);

	unsigned long groupCount = MAX(ceilf(list1Count / (float)clampedList1ItemsPerGroup), ceilf(list2Count / (float)clampedList2ItemsPerGroup));
	unsigned long outputCount = ((list1Count > 0 ? clampedList1ItemsPerGroup : 0)
							   + (list2Count > 0 ? clampedList2ItemsPerGroup : 0)) * groupCount;
	if (outputCount == 0)
	{
		*interleavedList = NULL;
		return;
	}

	VuoGenericType1 *list1Data = VuoListGetData_VuoGenericType1(list1);
	VuoGenericType1 *list2Data = VuoListGetData_VuoGenericType1(list2);

	*interleavedList = VuoListCreateWithCount_VuoGenericType1(outputCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *interleavedListData = VuoListGetData_VuoGenericType1(*interleavedList);
	unsigned long m = 0;
	for (unsigned long group = 0; group < groupCount; ++group)
	{
		if (list1Count)
			for (unsigned long i = 0; i < clampedList1ItemsPerGroup; ++i)
				interleavedListData[m++] = list1Data[MIN(group * clampedList1ItemsPerGroup + i, list1Count - 1)];

		if (list2Count)
			for (unsigned long i = 0; i < clampedList2ItemsPerGroup; ++i)
				interleavedListData[m++] = list2Data[MIN(group * clampedList2ItemsPerGroup + i, list2Count - 1)];
	}
}
