/**
 * @file
 * vuo.file.create.folder node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include "node.h"

VuoModuleMetadata({
					  "title" : "Create Folder",
					  "keywords" : [ "directory", "make", "new", "mkdir" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoText",
						  "VuoUrl"
					  ],
					  "node": {
						  "exampleCompositions" : [ ],
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"~/Desktop/MyFolder", "name":"Create at URL", "isSave":true}) createAtUrl,
		VuoInputEvent({"eventBlocking":"none", "data":"createAtUrl"}) createAtUrlEvent,
		VuoOutputEvent() done
)
{
	if (!createAtUrl || !createAtUrlEvent)
		return;

	VuoUrl folderUrl = VuoUrl_normalize(createAtUrl, VuoUrlNormalize_default);
	VuoRetain(folderUrl);
	VuoText folderPath = VuoUrl_getPosixPath(folderUrl);
	VuoRetain(folderPath);
	if (!folderPath)
	{
		VUserLog("Error: Couldn't create folder '%s' because remote URLs are not supported.", folderUrl);
		goto cleanup;
	}

	struct stat sb;
	if (stat(folderPath, &sb) == 0)  // Does folderPath exist?
	{
		if (! S_ISDIR(sb.st_mode))
			VUserLog("Error: Couldn't create folder '%s' because a file already exists at that URL.", folderUrl);
		goto cleanup;
	}

	VuoList_VuoText foldersToCreate = VuoListCreate_VuoText();
	VuoRetain(foldersToCreate);
	VuoText currFolder = folderPath;
	while (1)
	{
		VuoListAppendValue_VuoText(foldersToCreate, currFolder);
		char *parentFolder = dirname((char *)currFolder);
		if (!parentFolder || strlen(parentFolder) <= 1 || stat(parentFolder, &sb) == 0)
			break;
		currFolder = VuoText_make(parentFolder);
	}

	unsigned long count = VuoListGetCount_VuoText(foldersToCreate);
	for (unsigned long i = count; i >= 1; --i)
	{
		VuoText currFolder = VuoListGetValue_VuoText(foldersToCreate, i);

		// `mkdir` ANDs the mode with the process's umask,
		// so by default the created directory's mode will end up being 0755.
		int ret = mkdir(currFolder, 0777);
		if (ret)
		{
			VUserLog("Error: Couldn't create folder '%s' : %s", currFolder, strerror(errno));
			break;
		}
	}
	VuoRelease(foldersToCreate);

cleanup:
	VuoRelease(folderUrl);
	VuoRelease(folderPath);
}
