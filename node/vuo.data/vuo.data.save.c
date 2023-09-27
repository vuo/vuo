/**
 * @file
 * vuo.data.save node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include "VuoData.h"

VuoModuleMetadata({
					  "title" : "Save Data",
					  "keywords" : [ "write", "file", "output", "append", "store", "txt", "text", "csv", "xml" ],
					  "version" : "1.1.0",
					  "dependencies" : [
						  "VuoUrl",
					  ],
					  "node": {
						  "exampleCompositions": [ "SaveWordDefinition.vuo" ],
					  }
				  });

void nodeEvent
(
	// @todo use VuoUrl whenever it gets an input editor.
	VuoInputData(VuoText, { "default":"~/Desktop/MyData.txt", "name":"URL", "isSave":true }) url,
	VuoInputData(VuoData) saveData,
	VuoInputEvent({"eventBlocking":"none","data":"saveData"}) saveDataEvent,
	VuoInputData(VuoInteger, {"default":0, "name":"If Exists", "menuItems":[
		{"value":0, "name":"Don't Save"},
		{"value":1, "name":"Overwrite"},
		{"value":2, "name":"Append"},
	]}) overwriteUrl,
	VuoOutputEvent() done
)
{
	if (saveDataEvent)
	{
		VuoUrl normalized_url = VuoUrl_normalize(url, VuoUrlNormalize_forSaving);
		VuoLocal(normalized_url);
		VuoText absolute_path = VuoUrl_getPosixPath(normalized_url);
		VuoLocal(absolute_path);

		FILE* file = NULL;

		if (overwriteUrl == 0) // Don't Save
		{
			file = fopen(absolute_path, "r");

			if(file != NULL)
			{
				fclose(file);
				return;
			}
		}

		const char *mode = "wb";
		if (overwriteUrl == 2) // Append
			mode = "ab";

		file = fopen(absolute_path, mode);

		if(file != NULL)
		{
			if (saveData.data != NULL)
				fwrite(saveData.data, sizeof(char), (size_t)saveData.size, file);
			fclose(file);
		}
	}
}
