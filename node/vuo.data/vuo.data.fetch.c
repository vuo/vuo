/**
 * @file
 * vuo.data.fetch node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoUrlFetch.h"
#include "VuoData.h"

VuoModuleMetadata({
					 "title" : "Fetch Data",
					 "keywords" : [
						 "download", "open", "load", "import", "http", "url", "file", "get",
						 "text", "binary",
						 "csv", "tsv", "xml", "json",
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoUrlFetch"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "ShowBarChart.vuo" ]
					 }
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
		*data = VuoData_make(dataLength, dataPointer);
	else
		*data = VuoData_make(0, NULL);
}
