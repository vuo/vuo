/**
 * @file
 * vuo.data.save node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdio.h>
#include "VuoData.h"

VuoModuleMetadata({
					  "title" : "Save Data",
					  "keywords" : [ "write", "file", "output", "store", "txt", "text", "csv", "xml" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrl",
					  ],
					  "node": {
						  "isInterface": true,
						  "exampleCompositions": [ "SaveWordOfTheDay.vuo" ],
					  }
				  });

void nodeEvent
(
	// @todo use VuoUrl whenever it gets an input editor.
	VuoInputData(VuoText, { "default":"~/Desktop/MyData.txt", "name":"URL" }) url,
	VuoInputData(VuoData) saveData,
	VuoInputEvent({"eventBlocking":"none","data":"saveData"}) saveDataEvent,
	VuoInputData(VuoBoolean, { "default":false, "name":"Overwrite URL" }) overwriteUrl,
	VuoOutputEvent() done
)
{
	if (saveDataEvent)
	{
		VuoUrl normalized_url = VuoUrl_normalize(url, true);
		VuoRetain(normalized_url);
		VuoText absolute_path = VuoUrl_getPosixPath(normalized_url);
		VuoRetain(absolute_path);
		VuoDefer(^{ VuoRelease(absolute_path); });
		VuoRelease(normalized_url);

		FILE* file = NULL;

		if(!overwriteUrl)
		{
			file = fopen(absolute_path, "r");

			if(file != NULL)
			{
				fclose(file);
				return;
			}
		}

		file = fopen(absolute_path, "wb");

		if(file != NULL)
		{
			if (saveData.data != NULL)
				fwrite(saveData.data, sizeof(char), (size_t)saveData.size, file);
			fclose(file);
		}
	}
}
