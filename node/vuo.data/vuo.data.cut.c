/**
 * @file
 * vuo.data.cut node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoData.h"

VuoModuleMetadata({
	"title": "Cut Data",
	"keywords": [
		"binary",
		"part", "piece", "truncate", "trim", "subrange", "extract", "size"
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoData) data,
	VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) startByte,
	VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) byteCount,
	VuoOutputData(VuoData) partialData)
{
	if (!data.data || data.size <= 0)
	{
		partialData->size = 0;
		partialData->data = NULL;
		return;
	}

	signed long clampedStartByte = startByte - 1;
	signed long clampedEndByte = clampedStartByte + byteCount - 1;

	if (clampedStartByte < 0)
		clampedStartByte = 0;
	if (clampedEndByte >= data.size)
		clampedEndByte = data.size - 1;

	if (clampedStartByte > clampedEndByte)
	{
		partialData->size = 0;
		partialData->data = NULL;
		return;
	}

	partialData->size = clampedEndByte - clampedStartByte + 1;
	partialData->data = malloc(partialData->size);
	VuoRegister(partialData->data, free);
	memcpy(partialData->data, data.data + clampedStartByte, partialData->size);
}
