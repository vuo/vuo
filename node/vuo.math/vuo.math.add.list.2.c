/**
 * @file
 * vuo.math.add.list.2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Add Lists",
					  "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate", "vector", "point", "combine" ],
					  "version" : "1.0.1",
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
		VuoInputData(VuoList_VuoGenericType1, {"name":"Values 1"}) list1,
		VuoInputData(VuoList_VuoGenericType1, {"name":"Values 2"}) list2,
		VuoOutputData(VuoList_VuoGenericType1, {"name":"Sums"}) summedList
)
{
	unsigned long list1Count = VuoListGetCount_VuoGenericType1(list1);
	unsigned long list2Count = VuoListGetCount_VuoGenericType1(list2);
	unsigned long maxCount = MAX(list1Count, list2Count);

	VuoGenericType1 *list1Data = VuoListGetData_VuoGenericType1(list1);
	VuoGenericType1 *list2Data = VuoListGetData_VuoGenericType1(list2);

	*summedList = VuoListCreateWithCount_VuoGenericType1(maxCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *summedListData = VuoListGetData_VuoGenericType1(*summedList);

	VuoGenericType1 defaultValue = VuoGenericType1_makeFromJson(NULL);
	for (unsigned long i = 0; i < maxCount; ++i)
	{
		if (i < list1Count && i < list2Count)
			summedListData[i] = list1Data[i] + list2Data[i];
		else if (i < list1Count)
			summedListData[i] = list1Data[i];
		else if (i < list2Count)
			summedListData[i] = list2Data[i];
		else
			summedListData[i] = defaultValue;
	}
}
