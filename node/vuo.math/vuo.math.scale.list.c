/**
 * @file
 * vuo.math.scale.list node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Scale List",
					  "keywords" : [ "multiply", "product", "times", "*", "•", "×", "x", "arithmetic", "calculate" ],
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

void nodeEvent
(
		VuoInputData(VuoGenericType1) scale,
		VuoInputData(VuoList_VuoGenericType1, {"name":"Values"}) list,
		VuoOutputData(VuoList_VuoGenericType1, {"name":"Scaled Values"}) scaledList
)
{
	unsigned long listCount = VuoListGetCount_VuoGenericType1(list);
	if (listCount == 0)
	{
		*scaledList = NULL;
		return;
	}

	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);
	*scaledList = VuoListCreateWithCount_VuoGenericType1(listCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *scaledListData = VuoListGetData_VuoGenericType1(*scaledList);
	for (unsigned long i = 0; i < listCount; ++i)
		scaledListData[i] = listData[i] * scale;
}
