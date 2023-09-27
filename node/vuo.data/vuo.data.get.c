/**
 * @file
 * vuo.data.get node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoData.h"

VuoModuleMetadata({
	"title": "Get Data Bytes",
	"keywords": [ "binary" ],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoData) data,
	VuoOutputData(VuoList_VuoInteger) bytes)
{
	if (!data.data || data.size <= 0)
	{
		*bytes = NULL;
		return;
	}

	*bytes = VuoListCreateWithCount_VuoInteger(data.size, 0);
	VuoInteger *bytesArray = VuoListGetData_VuoInteger(*bytes);
	for (unsigned long i = 0; i < data.size; ++i)
		bytesArray[i] = (unsigned char)data.data[i];
}
