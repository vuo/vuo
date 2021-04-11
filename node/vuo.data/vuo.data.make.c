/**
 * @file
 * vuo.data.make node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoData.h"

VuoModuleMetadata({
	"title": "Make Data from Bytes",
	"keywords": [ "binary" ],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoInteger) bytes,
	VuoOutputData(VuoData) data)
{
	data->size = VuoListGetCount_VuoInteger(bytes);
	if (data->size == 0)
	{
		data->data = NULL;
		return;
	}

	data->data = malloc(data->size);
	VuoRegister(data->data, free);
	VuoInteger *bytesArray = VuoListGetData_VuoInteger(bytes);
	for (unsigned long i = 0; i < data->size; ++i)
		data->data[i] = VuoInteger_clamp(bytesArray[i], 0, 255);
}
