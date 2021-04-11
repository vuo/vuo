/**
 * @file
 * vuo.table.fetch node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"
#include "VuoUrlFetch.h"

VuoModuleMetadata({
					  "title" : "Fetch Table",
					  "keywords" : [
						"download", "open", "load", "import", "http", "url", "file", "get", "read", "make",
						"row", "column", "grid", "spreadsheet", "structure",
						"csv", "tsv", "comma", "tab", "separated",
						"parse", "convert", "read"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrlFetch",
					  ],
					  "node" : {
						  "exampleCompositions" : [ "DisplayPetAdoptionsTable.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoInputData(VuoTableFormat, {"default":"csv"}) format,
		VuoOutputData(VuoTable) table
)
{
	void *data;
	unsigned int dataLength;
	if (VuoUrl_fetch(url, &data, &dataLength))
	{
		// Include the NULL terminating byte allocated and initialized by VuoUrl_fetch().
		VuoText text = VuoText_makeFromData(data, dataLength + 1);
		free(data);

		*table = VuoTable_makeFromText(text, format);
	}
	else
		*table = VuoTable_makeEmpty();
}
