/**
 * @file
 * vuo.file.list node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include "node.h"
#include "VuoFileType.h"
#include "VuoFileListSort.h"
#include "VuoUrlFetch.h"

VuoModuleMetadata({
					  "title" : "List Files",
					  "keywords" : [ "folder", "directory", "path", "url",
						  "search", "scanner", "find"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrlFetch",
						  "VuoFileListSort"
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
			VLog("Error: Couldn't open folder '%s'.", folderPath);
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
		VuoText parts[3] = { folderPath, "/", VuoListGetValue_VuoText(topLevelFileNames, i) };
		VuoText filePath = VuoText_append(parts, 3);

		bool isDir = false;
		struct stat st_buf;
		int status = lstat(filePath, &st_buf);  // Unlike stat, lstat doesn't follow symlinks.
		if (! status && S_ISDIR(st_buf.st_mode))
			isDir = true;

		if (isDir)
		{
			if (fileType == VuoFileType_Folder)
				VuoListAppendValue_VuoText(files, filePath);

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
			if (VuoFileType_isFileOfType(filePath, fileType))
				VuoListAppendValue_VuoText(files, filePath);
		}

		VuoRetain(filePath);
		VuoRelease(filePath);
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
	VuoText folderUrl = VuoUrl_normalize(folder, false);

	const char *fileProtocol = "file://";
	VuoText protocol = VuoText_substring(folderUrl, 1, strlen(fileProtocol));
	if (! VuoText_areEqual(protocol, fileProtocol))
	{
		VLog("Error: Couldn't list files in '%s' because remote URLs are not supported.", folderUrl);
		*files = VuoListCreate_VuoText();
		goto cleanup;
	}

	VuoText folderPath = VuoText_substring(folderUrl, strlen(fileProtocol)+1, VuoText_length(folderUrl));
	*files = listFiles(folderPath, includeSubfolders, fileType);

cleanup:
	VuoRetain(protocol);
	VuoRelease(protocol);
	VuoRetain(folderUrl);
	VuoRelease(folderUrl);
	VuoRetain(folderPath);
	VuoRelease(folderPath);
}
