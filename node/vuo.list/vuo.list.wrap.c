/**
 * @file
 * vuo.list.wrap node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Wrap List",
	"keywords": [
		"rotate", "shift", "loop", "cycle", "move", "rearrange", "displace", "repeat",
		"array", "set", "sequence",
		"items", "index", "indices",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoGenericType1) list,
	VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) startPosition,
	VuoOutputData(VuoList_VuoGenericType1) wrappedList)
{
	VuoInteger listSize = VuoListGetCount_VuoGenericType1(list);
	if (listSize < 1)
	{
		*wrappedList = NULL;
		return;
	}

	VuoGenericType1 *listData = VuoListGetData_VuoGenericType1(list);
	*wrappedList = VuoListCreateWithCount_VuoGenericType1(listSize, VuoGenericType1_makeFromJson(NULL));
	VuoGenericType1 *wrappedListData = VuoListGetData_VuoGenericType1(*wrappedList);
	for (VuoInteger i = 0; i < listSize; ++i)
	{
		wrappedListData[i] = listData[VuoInteger_wrap(i + startPosition - 1, 0, listSize - 1)];
		VuoGenericType1_retain(wrappedListData[i]);
	}
}
