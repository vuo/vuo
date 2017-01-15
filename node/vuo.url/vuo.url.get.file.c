/**
 * @file
 * vuo.url.get.file node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get File URL Values",
					  "keywords" : [ "split", "tokenize", "explode", "parts", "piece" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrl"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ShowIconsAndFilenames.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoOutputData(VuoText) path,
		VuoOutputData(VuoText) folder,
		VuoOutputData(VuoText) fileName,
		VuoOutputData(VuoText) extension
)
{
	if (!VuoUrl_getFileParts(url, path, folder, fileName, extension))
	{
//		VUserLog("Warning: Couldn't parse \"%s\"", url);
		*path = *folder = *fileName = *extension = NULL;
	}
}
