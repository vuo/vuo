/**
 * @file
 * vuo.data.append node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoData.h"
#include "VuoList_VuoData.h"

VuoModuleMetadata({
	"title": "Append Data",
	"keywords": [
		"binary",
		"push", "append", "prepend", "insert", "combine", "concatenate", "join", "together", "merge",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoData) data,
	VuoOutputData(VuoData) compositeData)
{
	unsigned long dataCount = VuoListGetCount_VuoData(data);
	VuoData *dataArray = VuoListGetData_VuoData(data);
	unsigned long totalSize = 0;
	for (unsigned long i = 0; i < dataCount; ++i)
		totalSize += dataArray[i].size;
	if (!totalSize)
	{
		compositeData->size = 0;
		compositeData->data = NULL;
		return;
	}

	compositeData->size = totalSize;
	compositeData->data = malloc(totalSize);
	VuoRegister(compositeData->data, free);
	unsigned long pos = 0;
	for (unsigned long i = 0; i < dataCount; pos += dataArray[i].size, ++i)
		memcpy(compositeData->data + pos, dataArray[i].data, dataArray[i].size);
}
