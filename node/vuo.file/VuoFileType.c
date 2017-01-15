/**
 * @file
 * VuoFileType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include "type.h"
#include "VuoFileFormat.h"
#include "VuoFileType.h"
#include "VuoList_VuoFileType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "File Type",
					  "description" : "The type of information in a file.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoFileType"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoFileType
 * Decodes the JSON object @a js to create a new value.
 */
VuoFileType VuoFileType_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "any") == 0)
		return VuoFileType_AnyFile;
	else if (strcmp(valueAsString, "audio") == 0)
		return VuoFileType_Audio;
	else if (strcmp(valueAsString, "image") == 0)
		return VuoFileType_Image;
	else if (strcmp(valueAsString, "mesh") == 0)
		return VuoFileType_Mesh;
	else if (strcmp(valueAsString, "movie") == 0)
		return VuoFileType_Movie;
	else if (strcmp(valueAsString, "scene") == 0)
		return VuoFileType_Scene;
	else if (strcmp(valueAsString, "feed") == 0)
		return VuoFileType_Feed;
	else if (strcmp(valueAsString, "folder") == 0)
		return VuoFileType_Folder;

	return VuoFileType_AnyFile;
}

/**
 * @ingroup VuoFileType
 * Encodes @a value as a JSON object.
 */
json_object * VuoFileType_getJson(const VuoFileType value)
{
	char *valueAsString = "any";

	if (value == VuoFileType_Audio)
		valueAsString = "audio";
	else if (value == VuoFileType_Image)
		valueAsString = "image";
	else if (value == VuoFileType_Mesh)
		valueAsString = "mesh";
	else if (value == VuoFileType_Movie)
		valueAsString = "movie";
	else if (value == VuoFileType_Scene)
		valueAsString = "scene";
	else if (value == VuoFileType_Feed)
		valueAsString = "feed";
	else if (value == VuoFileType_Folder)
		valueAsString = "folder";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoFileType VuoFileType_getAllowedValues(void)
{
	VuoList_VuoFileType l = VuoListCreate_VuoFileType();
	VuoListAppendValue_VuoFileType(l, VuoFileType_AnyFile);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Audio);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Image);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Mesh);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Movie);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Scene);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Feed);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Folder);
	return l;
}

/**
 * @ingroup VuoFileType
 * Returns a compact string representation of @a value.
 */
char * VuoFileType_getSummary(const VuoFileType value)
{
	char *valueAsString = "Any File";

	if (value == VuoFileType_Audio)
		valueAsString = "Audio";
	else if (value == VuoFileType_Image)
		valueAsString = "Image";
	else if (value == VuoFileType_Mesh)
		valueAsString = "Projection Mesh";
	else if (value == VuoFileType_Movie)
		valueAsString = "Movie";
	else if (value == VuoFileType_Scene)
		valueAsString = "Scene";
	else if (value == VuoFileType_Feed)
		valueAsString = "Feed";
	else if (value == VuoFileType_Folder)
		valueAsString = "Folder";

	return strdup(valueAsString);
}

/**
 * @ingroup VuoFileType
 * Returns true if the file at @a path (assumed to be a file, not a folder) is of type @a fileType.
 */
bool VuoFileType_isFileOfType(const VuoText path, VuoFileType fileType)
{
	if (fileType == VuoFileType_AnyFile)
		return true;
	else if (fileType == VuoFileType_Audio)
		return VuoFileFormat_isSupportedAudioFile(path);
	else if (fileType == VuoFileType_Image)
		return VuoFileFormat_isSupportedImageFile(path);
	else if (fileType == VuoFileType_Mesh)
		return VuoFileFormat_isSupportedMeshFile(path);
	else if (fileType == VuoFileType_Movie)
		return VuoFileFormat_isSupportedMovieFile(path);
	else if (fileType == VuoFileType_Scene)
		return VuoFileFormat_isSupportedSceneFile(path);
	else if (fileType == VuoFileType_Feed)
		return VuoFileFormat_isSupportedFeedFile(path);

	return false;
}
