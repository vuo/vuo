/**
 * @file
 * vuo.list.spread.group node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					"title" : "Spread List Item Groups",
					"keywords" : [ "duplicate", "copy", "clone", "double", "replicate", "expand", "fill", "stretch",
						"cycle", "stride"
					],
					"version" : "1.0.0",
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
	*outputList = VuoListCreate_VuoGenericType1();
	unsigned long inputCount = VuoListGetCount_VuoGenericType1(list);
	for (unsigned long i = 1; i <= inputCount; i += itemsPerGroup)
		for (unsigned long j = 0; j < copies; ++j)
			for (unsigned long k = 0; k < itemsPerGroup; ++k)
				VuoListAppendValue_VuoGenericType1(*outputList, VuoListGetValue_VuoGenericType1(list, i + k));
}
