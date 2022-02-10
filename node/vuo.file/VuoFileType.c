/**
 * @file
 * VuoFileType implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "type.h"
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
 * A map of the extensions for each file type.
 */
static const char *VuoFileType_extensionsJSON = VUO_STRINGIFY({
	"audio": ["wav", "aif", "aiff", "mp3", "mp2", "aac", "m4a", "ac3", "3gp", "amr"],
	"image": ["png", "jpeg", "jpg", "gif", "bmp", "exr", "hdr", "psd", "raw", "cr2",
			  "dng", "dcr", "nef", "raf", "mos", "kdc", "tif", "tiff", "tga", "targa", "webp", "pct"],
	"mesh":  ["data"],  // PBMesh projection mesh
	"movie": ["mov", "avi", "dv", "mpeg", "mpg", "mp2", "m4v", "mp4", "ogv", "gif", "qt"],
	"scene": ["3ds", "dae", "obj", "dxf", "ply", "lwo", "lxo", "ac3d", "ms3d", "cob", "scn",
			  "irr", "irrmesh", "mdl", "md2", "md3", "pk3", "mdc", "md5", "m3", "smd", "ter",
			  "raw", "b3d", "q3d", "q3s", "nff", "off", "3dgs", "hmp", "ndo", "fbx", "blend", "stl"],
	"feed":  ["rss", "rdf"],
	"app":   ["app", "app/"],
	"data":  ["icc", "txt"],
	"json":  ["json"],
	"table": ["csv", "tsv"],
	"xml":   ["xml"],
});

static json_object *VuoFileType_extensions = NULL;  ///< VuoFileType_extensionsJSON parsed.
static dispatch_once_t VuoFileType_extensionsInitialized = 0;  ///< Whether VuoFileType_extensions has been initialized.

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
	else if (strcmp(valueAsString, "folder") == 0)
		return VuoFileType_Folder;
	else if (strcmp(valueAsString, "feed") == 0)
		return VuoFileType_Feed;
	else if (strcmp(valueAsString, "app") == 0)
		return VuoFileType_App;
	else if (strcmp(valueAsString, "data") == 0)
		return VuoFileType_Data;
	else if (strcmp(valueAsString, "json") == 0)
		return VuoFileType_JSON;
	else if (strcmp(valueAsString, "table") == 0)
		return VuoFileType_Table;
	else if (strcmp(valueAsString, "xml") == 0)
		return VuoFileType_XML;

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
	else if (value == VuoFileType_Folder)
		valueAsString = "folder";
	else if (value == VuoFileType_Feed)
		valueAsString = "feed";
	else if (value == VuoFileType_App)
		valueAsString = "app";
	else if (value == VuoFileType_Data)
		valueAsString = "data";
	else if (value == VuoFileType_JSON)
		valueAsString = "json";
	else if (value == VuoFileType_Table)
		valueAsString = "table";
	else if (value == VuoFileType_XML)
		valueAsString = "xml";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoFileType VuoFileType_getAllowedValues(void)
{
	VuoList_VuoFileType l = VuoListCreate_VuoFileType();
	VuoListAppendValue_VuoFileType(l, VuoFileType_AnyFile);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Folder);

	VuoListAppendValue_VuoFileType(l, VuoFileType_App);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Audio);
//  VuoListAppendValue_VuoFileType(l, VuoFileType_Data);  // Omitted since it isn't useful to use `List Files` on this assortment of file types.
	VuoListAppendValue_VuoFileType(l, VuoFileType_Feed);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Image);
	VuoListAppendValue_VuoFileType(l, VuoFileType_JSON);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Mesh);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Movie);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Scene);
	VuoListAppendValue_VuoFileType(l, VuoFileType_Table);
	VuoListAppendValue_VuoFileType(l, VuoFileType_XML);
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
	else if (value == VuoFileType_Folder)
		valueAsString = "Folder";
	else if (value == VuoFileType_Feed)
		valueAsString = "Feed";
	else if (value == VuoFileType_App)
		valueAsString = "App";
	else if (value == VuoFileType_Data)
		valueAsString = "Data";
	else if (value == VuoFileType_JSON)
		valueAsString = "JSON";
	else if (value == VuoFileType_Table)
		valueAsString = "Table";
	else if (value == VuoFileType_XML)
		valueAsString = "XML";

	return strdup(valueAsString);
}

/**
 * Parses the list of file type extensions.
 */
static void VuoFileType_initExtensions(void)
{
	dispatch_once(&VuoFileType_extensionsInitialized, ^{
		VuoFileType_extensions = json_tokener_parse(VuoFileType_extensionsJSON);
	});
}

/**
 * @ingroup VuoFileType
 * Returns true if `path` is of type `fileType`, based on the file extension.
 * This function does not check whether `path` exists, or differentiate between files and folders.
 */
bool VuoFileType_isFileOfType(const VuoText path, VuoFileType fileType)
{
	if (fileType == VuoFileType_AnyFile)
		return true;

	const char *dot = strrchr(path, '.');
	if (!dot)
		return false;

	const char *actualExtension = dot + 1;

	VuoFileType_initExtensions();
	json_object *extensions = VuoFileType_getExtensions(fileType);
	if (!extensions)
		return false;

	int extensionCount = json_object_array_length(extensions);
	for (int i = 0; i < extensionCount; ++i)
		if (strcasecmp(actualExtension, json_object_get_string(json_object_array_get_idx(extensions, i))) == 0)
			return true;

	return false;
}

/**
 * Returns a JSON array of file extensions for the specified `fileType`.
 *
 * Do not modify or free the returned object.
 */
json_object *VuoFileType_getExtensions(VuoFileType fileType)
{
	VuoFileType_initExtensions();
	json_object *fileTypeJson = VuoFileType_getJson(fileType);
	json_object *extensions = NULL;
	json_object_object_get_ex(VuoFileType_extensions, json_object_get_string(fileTypeJson), &extensions);
	json_object_put(fileTypeJson);
	return extensions;
}
