/**
 * @file
 * vuo.file.exists node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include "VuoUrlFetch.h"

VuoModuleMetadata({
					  "title" : "File Exists",
					  "keywords" : [
						  "folder", "directory", "path", "url",
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrl"
					  ],
					  "node": {
						  "exampleCompositions" : [ ],
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL","default":""}) url,
		VuoOutputData(VuoBoolean) exists
)
{
	if (VuoText_isEmpty(url))
	{
		*exists = false;
		return;
	}

	VuoUrl normalizedUrl = VuoUrl_normalize(url, VuoUrlNormalize_default);
	VuoLocal(normalizedUrl);

	VuoText posixPath = VuoUrl_getPosixPath(normalizedUrl);
	VuoLocal(posixPath);

	struct stat s;
	*exists = (stat(posixPath, &s) == 0);
}
