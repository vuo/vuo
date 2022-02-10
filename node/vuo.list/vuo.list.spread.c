/**
 * @file
 * vuo.list.spread node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					"title" : "Copy List Items",
					"keywords" : [ "duplicate", "copy", "clone", "double", "repeat", "replicate", "expand", "fill", "stretch" ],
					"version" : "1.0.2",
					"node": {
						"exampleCompositions" : [ "StringRepeatingBeads.vuo" ]
					}
				});

void nodeEvent
(
	VuoInputData(VuoList_VuoGenericType1) list,
	VuoInputData(VuoInteger, {"default":2,"suggestedMin":1}) copies,
	VuoOutputData(VuoList_VuoGenericType1) outputList
)
{
	unsigned long inputCount = VuoListGetCount_VuoGenericType1(list);
	unsigned long clampedCopies = MAX(0,copies);
	unsigned long outputCount = inputCount * clampedCopies;
	if (outputCount == 0)
	{
		*outputList = NULL;
		return;
	}
	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);
	*outputList = VuoListCreateWithCount_VuoGenericType1(outputCount, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *outputListData = VuoListGetData_VuoGenericType1(*outputList);
	for (unsigned long i = 0; i < inputCount; ++i)
		for (unsigned long j = 0; j < clampedCopies; ++j)
		{
			outputListData[i * clampedCopies + j] = listData[i];
			VuoGenericType1_retain(outputListData[i * clampedCopies + j]);
		}
}
