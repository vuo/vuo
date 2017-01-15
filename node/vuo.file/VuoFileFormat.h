/**
 * @file
 * VuoFileFormat interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Returns true if the file has one of the given formats, based on the file extension.
 */
bool VuoFileFormat_isFileOfFormat(const char *path, const char **formats, size_t numFormats)
{
	/// @todo https://b33p.net/kosada/node/8795 : Determine the file type from its content.

	const char *dot = strrchr(path, '.');
	if (! dot)
		return false;
	const char *rawExtension = dot + 1;

	size_t extensionLength = strlen(rawExtension);
	char *extension = (char *)malloc(extensionLength + 1);
	for (size_t i = 0; i < extensionLength; ++i)
		extension[i] = tolower(*(rawExtension + i));
	extension[extensionLength] = 0;

	bool found = false;
	for (int i = 0; i < numFormats && ! found; ++i)
		if (strcmp(extension, formats[i]) == 0)
			found = true;

	free(extension);

	return found;
}

/**
 * Returns true if the file is of one of the supported audio file formats.
 */
bool VuoFileFormat_isSupportedAudioFile(const char *path)
{
	const char *formats[] = {"wav", "aif", "aiff", "mp3", "mp2", "aac", "m4a", "ac3", "3gp", "amr"};
	size_t numFormats = sizeof(formats)/sizeof(formats[0]);
	return VuoFileFormat_isFileOfFormat(path, formats, numFormats);
}

/**
 * Returns true if the file is of one of the supported image file formats.
 */
bool VuoFileFormat_isSupportedImageFile(const char *path)
{
	const char *formats[] = {"png", "jpeg", "jpg", "gif", "bmp", "exr", "hdr", "psd", "raw", "cr2",
							 "dng", "dcr", "nef", "raf", "mos", "kdc", "tif", "tiff", "tga"};
	size_t numFormats = sizeof(formats)/sizeof(formats[0]);
	return VuoFileFormat_isFileOfFormat(path, formats, numFormats);
}

/**
 * Returns true if the file is of one of the supported mesh file formats.
 */
bool VuoFileFormat_isSupportedMeshFile(const char *path)
{
	const char *formats[] = {"data"};
	size_t numFormats = sizeof(formats)/sizeof(formats[0]);
	return VuoFileFormat_isFileOfFormat(path, formats, numFormats);
}

/**
 * Returns true if the file is of one of the supported movie file formats.
 */
bool VuoFileFormat_isSupportedMovieFile(const char *path)
{
	const char *formats[] = {"mov", "avi", "dv", "mpeg", "mpg", "mp2", "m4v", "mp4", "ogv", "gif"};
	size_t numFormats = sizeof(formats)/sizeof(formats[0]);
	return VuoFileFormat_isFileOfFormat(path, formats, numFormats);
}

/**
 * Returns true if the file is of one of the supported scene file formats.
 */
bool VuoFileFormat_isSupportedSceneFile(const char *path)
{
	const char *formats[] = {"3ds", "dae", "obj", "dxf", "ply", "lwo", "lxo", "ac3d", "ms3d", "cob", "scn", "xml",
							 "irr", "irrmesh", "mdl", "md2", "md3", "pk3", "mdc", "md5", "m3", "smd", "ter",
							 "raw", "b3d", "q3d", "q3s", "nff", "off", "3dgs", "hmp", "ndo", "fbx", "blend", "stl"};
	size_t numFormats = sizeof(formats)/sizeof(formats[0]);
	return VuoFileFormat_isFileOfFormat(path, formats, numFormats);
}

/**
 * Returns true if the file is of one of the supported RSS file formats.
 */
bool VuoFileFormat_isSupportedFeedFile(const char *path)
{
	const char *formats[] = {"rss", "rdf"};
	size_t numFormats = sizeof(formats)/sizeof(formats[0]);
	return VuoFileFormat_isFileOfFormat(path, formats, numFormats);
}

/**
 * Returns true if the file is of one of the supported data file formats.
 */
bool VuoFileFormat_isSupportedDataFile(const char *path)
{
	const char *formats[] = {"txt", "csv"};
	size_t numFormats = sizeof(formats)/sizeof(formats[0]);
	return VuoFileFormat_isFileOfFormat(path, formats, numFormats);
}

#ifdef __cplusplus
}
#endif
