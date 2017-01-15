/**
 * @file
 * vuo.file.list node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include "node.h"
#include "VuoFileType.h"
#include "VuoUrlFetch.h"

VuoModuleMetadata({
					  "title" : "List Files",
					  "keywords" : [ "folder", "directory", "path", "url",
						  "search", "scanner", "find"
					  ],
					  "version" : "1.0.2",
					  "dependencies" : [
						  "VuoUrlFetch"
					  ],
					  "node": {
						  "exampleCompositions" : [ ],
					  }
				  });


static VuoList_VuoText listFiles(VuoText folderPath, VuoBoolean includeSubfolders, VuoFileType fileType)
{
	VuoList_VuoText files = VuoListCreate_VuoText();

	DIR *d = opendir(folderPath);
	if (! d)
	{
		if (access(folderPath, F_OK) != -1)
			VUserLog("Error: Couldn't open folder '%s'.", folderPath);
		return files;
	}

	VuoList_VuoText topLevelFileNames = VuoListCreate_VuoText();
	struct dirent *de;
	while( (de=readdir(d)) )
	{
		if (de->d_name[0] != '.')  // Skip hidden files.
		{
			VuoText fileName = VuoText_make(de->d_name);
			VuoListAppendValue_VuoText(topLevelFileNames, fileName);
		}
	}
	closedir(d);

	VuoListSort_VuoText(topLevelFileNames);

	unsigned long count = VuoListGetCount_VuoText(topLevelFileNames);
	for (int i = 1; i <= count; ++i)
	{
		const char *fileUrlPrefix = "file://";
		size_t fileUrlPrefixLength = 7;
		VuoText parts[] = { fileUrlPrefix, VuoUrl_escapePosixPath(folderPath), "/", VuoUrl_escapePosixPath(VuoListGetValue_VuoText(topLevelFileNames, i)) };
		VuoText fileUrl = VuoText_append(parts, sizeof(parts)/sizeof(VuoText));
		VuoRetain(parts[1]);
		VuoRelease(parts[1]);
		VuoRetain(parts[3]);
		VuoRelease(parts[3]);

		bool isDir = false;
		VuoText filePath = VuoUrl_getPosixPath(fileUrl);
		if (!filePath)
			goto fail;

		struct stat st_buf;
		int status = lstat(filePath, &st_buf);  // Unlike stat, lstat doesn't follow symlinks.
		if (! status && S_ISDIR(st_buf.st_mode))
			isDir = true;

		if (isDir && !VuoUrl_isBundle(fileUrl))
		{
			if (fileType == VuoFileType_Folder)
				VuoListAppendValue_VuoText(files, fileUrl);

			if (includeSubfolders)
			{
				VuoList_VuoText filesInDir = listFiles(filePath, includeSubfolders, fileType);
				unsigned long fileCount = VuoListGetCount_VuoText(filesInDir);
				for (unsigned long i = 1; i <= fileCount; ++i)
					VuoListAppendValue_VuoText(files, VuoListGetValue_VuoText(filesInDir, i));
				VuoRetain(filesInDir);
				VuoRelease(filesInDir);
			}
		}
		else
		{
			if (VuoFileType_isFileOfType(fileUrl, fileType))
				VuoListAppendValue_VuoText(files, fileUrl);
		}

		VuoRetain(filePath);
		VuoRelease(filePath);

fail:
		VuoRetain(fileUrl);
		VuoRelease(fileUrl);
	}

	VuoRetain(topLevelFileNames);
	VuoRelease(topLevelFileNames);

	return files;
}

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) folder,
		VuoInputData(VuoBoolean, {"default":false}) includeSubfolders,
		VuoInputData(VuoFileType, {"default":"image"}) fileType,
		VuoOutputData(VuoList_VuoText) files
)
{
	if (!folder)
		return;

	VuoUrl folderUrl = VuoUrl_normalize(folder, false);
	VuoText folderPath = VuoUrl_getPosixPath(folderUrl);
	if (!folderPath)
	{
		VUserLog("Error: Couldn't list files in '%s' because remote URLs are not supported.", folderUrl);
		*files = VuoListCreate_VuoText();
		goto cleanup;
	}

	*files = listFiles(folderPath, includeSubfolders, fileType);

cleanup:
	VuoRetain(folderUrl);
	VuoRelease(folderUrl);
	VuoRetain(folderPath);
	VuoRelease(folderPath);
}
