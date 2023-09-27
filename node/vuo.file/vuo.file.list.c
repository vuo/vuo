/**
 * @file
 * vuo.file.list node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include "VuoFileType.h"
#include "VuoUrlFetch.h"

VuoModuleMetadata({
	"title": "List Files",
	"keywords": [
		"folder", "directory", "path", "url",
		"search", "scanner", "find",

		// File types that this node can find.
		// Keep in sync with `VuoFileType_extensionsJSON`.
		"audio", "sounds", "waves",
			"wav", "aif", "aiff", "mp3", "mp2", "aac", "m4a", "ac3", "3gp", "amr",
		"images", "photographs", "pictures", "bitmaps", "textures", "icons", "pngs", "jpegs", "jpgs", "gifs",
			"png", "jpeg", "jpg", "gif", "bmp", "exr", "hdr", "psd", "raw", "cr2",
			"dng", "dcr", "nef", "raf", "mos", "kdc", "tif", "tiff", "tga", "targa", "webp", "pct", "heic",
		"meshes", "data",
		"scenes", "objects", "objs",
			"3ds", "dae", "obj", "dxf", "ply", "lwo", "lxo", "ac3d", "ms3d", "cob", "scn",
			"irr", "irrmesh", "mdl", "md2", "md3", "pk3", "mdc", "md5", "m3", "smd", "ter",
			"raw", "b3d", "q3d", "q3s", "nff", "off", "3dgs", "hmp", "ndo", "fbx", "blend", "stl",
		"movies",
			"mov", "avi", "dv", "mpeg", "mpg", "mp2", "m4v", "mp4", "ogv", "gif", "qt",
		"feeds", "rss", "atom",
		"apps",
		"trees", "json", "xml",
		"tables", "csv", "tsv",
	],
	"version": "1.0.2",
	"dependencies": [
		"VuoUrlFetch",
	],
	"node": {
		"exampleCompositions": [ ],
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
				for (unsigned long j = 1; j <= fileCount; ++j)
					VuoListAppendValue_VuoText(files, VuoListGetValue_VuoText(filesInDir, j));
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

	VuoUrl folderUrl = VuoUrl_normalize(folder, VuoUrlNormalize_default);
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
