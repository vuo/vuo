/**
 * @file
 * vuo.data.fetch node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoUrlFetch.h"
#include "VuoData.h"

VuoModuleMetadata({
	"title": "Fetch Data",
	"keywords": [
		"download", "open", "load", "import", "http", "url", "web", "file", "get", "read",
		"text", "string", "binary",
		"csv", "tsv", "xml", "json",
	],
	"version": "1.0.0",
	"dependencies": [
		"VuoUrlFetch"
	],
	"node": {
		"exampleCompositions": [ "ShowBarChart.vuo" ],
	},
});

void nodeEvent
(
		VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
		VuoOutputData(VuoData) data
)
{
	void *dataPointer;
	unsigned int dataLength;
	if (VuoUrl_fetch(url, &dataPointer, &dataLength))
		// Include the NULL terminating byte allocated and initialized by VuoUrl_fetch().
		*data = VuoData_make(dataLength + 1, dataPointer);
	else
		*data = VuoData_make(0, NULL);
}
