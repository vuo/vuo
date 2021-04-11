/**
 * @file
 * vuo.list.spread.group node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					"title" : "Copy List Item Groups",
					"keywords" : [ "duplicate", "copy", "clone", "double", "repeat", "replicate", "expand", "fill", "stretch",
						"cycle", "stride"
					],
					"version" : "1.0.1",
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
	unsigned long clampedCopies = MAX(0,copies);
	unsigned long clampedItemsPerGroup = MAX(0,itemsPerGroup);
	*outputList = VuoListCreate_VuoGenericType1();
	unsigned long inputCount = VuoListGetCount_VuoGenericType1(list);
	for (unsigned long i = 1; i <= inputCount; i += itemsPerGroup)
		for (unsigned long j = 0; j < clampedCopies; ++j)
			for (unsigned long k = 0; k < clampedItemsPerGroup; ++k)
				VuoListAppendValue_VuoGenericType1(*outputList, VuoListGetValue_VuoGenericType1(list, i + k));
}
