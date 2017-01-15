/**
 * @file
 * vuo.list.spread node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					"title" : "Spread List Items",
					"keywords" : [ "duplicate", "copy", "clone", "double", "replicate", "expand", "fill", "stretch" ],
					"version" : "1.0.0",
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
	*outputList = VuoListCreate_VuoGenericType1();
	unsigned long inputCount = VuoListGetCount_VuoGenericType1(list);
	for (unsigned long i = 1; i <= inputCount; ++i)
		for (unsigned long j = 0; j < copies; ++j)
			VuoListAppendValue_VuoGenericType1(*outputList, VuoListGetValue_VuoGenericType1(list, i));
}
