/**
 * @file
 * VuoImageFormat implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageFormat.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Image Format",
					 "description" : "Available formats when exporting images from Vuo.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoImageFormat"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoImageFormat
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoImageFormat.
 */
VuoImageFormat VuoImageFormat_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoImageFormat value = VuoImageFormat_PNG;

	if (! strcmp(valueAsString, "PNG")) {
		value = VuoImageFormat_PNG;
	} else if (! strcmp(valueAsString, "JPEG")) {
		value = VuoImageFormat_JPEG;
	} else if (! strcmp(valueAsString, "TIFF")) {
		value = VuoImageFormat_TIFF;
	} else if (! strcmp(valueAsString, "BMP")) {
		value = VuoImageFormat_BMP;
	} else if (! strcmp(valueAsString, "HDR")) {
		value = VuoImageFormat_HDR;
	} else if (! strcmp(valueAsString, "EXR")) {
		value = VuoImageFormat_EXR;
	} else if (! strcmp(valueAsString, "GIF")) {
		value = VuoImageFormat_GIF;
	} else if (! strcmp(valueAsString, "TARGA")) {
		value = VuoImageFormat_TARGA;
	}
	else if (! strcmp(valueAsString, "WEBP"))
		value = VuoImageFormat_WEBP;

	return value;
}

/**
 * @ingroup VuoImageFormat
 * Encodes @c value as a JSON object.
 */
json_object * VuoImageFormat_getJson(const VuoImageFormat value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoImageFormat_PNG:
			valueAsString = "PNG";
			break;
		case VuoImageFormat_JPEG:
			valueAsString = "JPEG";
			break;
		case VuoImageFormat_TIFF:
			valueAsString = "TIFF";
			break;
		case VuoImageFormat_BMP:
			valueAsString = "BMP";
			break;
		case VuoImageFormat_HDR:
			valueAsString = "HDR";
			break;
		case VuoImageFormat_EXR:
			valueAsString = "EXR";
			break;
		case VuoImageFormat_GIF:
			valueAsString = "GIF";
			break;
		case VuoImageFormat_TARGA:
			valueAsString = "TARGA";
			break;
		case VuoImageFormat_WEBP:
			valueAsString = "WEBP";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoImageFormat VuoImageFormat_getAllowedValues(void)
{
	VuoList_VuoImageFormat l = VuoListCreate_VuoImageFormat();
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_PNG);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_JPEG);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_TIFF);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_BMP);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_HDR);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_EXR);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_GIF);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_TARGA);
	VuoListAppendValue_VuoImageFormat(l, VuoImageFormat_WEBP);
	return l;
}

/**
 * @ingroup VuoImageFormat
 * Same as @c %VuoImageFormat_getString() but with some capitilization involved.
 */
char * VuoImageFormat_getSummary(const VuoImageFormat value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoImageFormat_PNG:
			valueAsString = "PNG";
			break;
		case VuoImageFormat_JPEG:
			valueAsString = "JPEG";
			break;
		case VuoImageFormat_TIFF:
			valueAsString = "TIFF";
			break;
		case VuoImageFormat_BMP:
			valueAsString = "BMP";
			break;
		case VuoImageFormat_HDR:
			valueAsString = "HDR";
			break;
		case VuoImageFormat_EXR:
			valueAsString = "EXR";
			break;
		case VuoImageFormat_GIF:
			valueAsString = "GIF";
			break;
		case VuoImageFormat_TARGA:
			valueAsString = "TARGA";
			break;
		case VuoImageFormat_WEBP:
			valueAsString = "WEBP";
			break;
	}

	return strdup(valueAsString);
}

/**
 * @ingroup VuoImageFormat
 * Return an array of valid file extensions for this format (ex, JPEG returns {"jpeg", "jpg"}).
 */
struct json_object *VuoImageFormat_getValidFileExtensions(const VuoImageFormat format)
{
	struct json_object *js = json_object_new_array();

	if (format == VuoImageFormat_PNG)
		json_object_array_add(js, json_object_new_string("png"));
	else if (format == VuoImageFormat_JPEG)
	{
		json_object_array_add(js, json_object_new_string("jpg"));
		json_object_array_add(js, json_object_new_string("jpeg"));
	}
	else if (format == VuoImageFormat_TIFF)
	{
		json_object_array_add(js, json_object_new_string("tif"));
		json_object_array_add(js, json_object_new_string("tiff"));
	}
	else if (format == VuoImageFormat_BMP)
		json_object_array_add(js, json_object_new_string("bmp"));
	else if (format == VuoImageFormat_HDR)
		json_object_array_add(js, json_object_new_string("hdr"));
	else if (format == VuoImageFormat_EXR)
		json_object_array_add(js, json_object_new_string("exr"));
	else if (format == VuoImageFormat_GIF)
		json_object_array_add(js, json_object_new_string("gif"));
	else if (format == VuoImageFormat_TARGA)
	{
		json_object_array_add(js, json_object_new_string("tga"));
		json_object_array_add(js, json_object_new_string("targa"));
	}
	else if (format == VuoImageFormat_WEBP)
		json_object_array_add(js, json_object_new_string("webp"));

	return js;
}

/**
 * @ingroup VuoImageFormat
 * Return a char* that can be used as an extension.
 */
char * VuoImageFormat_getExtension(const VuoImageFormat value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoImageFormat_PNG:
			valueAsString = "png";
			break;
		case VuoImageFormat_JPEG:
			valueAsString = "jpeg";
			break;
		case VuoImageFormat_TIFF:
			valueAsString = "tiff";
			break;
		case VuoImageFormat_BMP:
			valueAsString = "bmp";
			break;
		case VuoImageFormat_HDR:
			valueAsString = "hdr";
			break;
		case VuoImageFormat_EXR:
			valueAsString = "exr";
			break;
		case VuoImageFormat_GIF:
			valueAsString = "gif";
			break;
		case VuoImageFormat_TARGA:
			valueAsString = "tga";
			break;
		case VuoImageFormat_WEBP:
			valueAsString = "webp";
			break;
	}

	return strdup(valueAsString);
}

/**
 * Returns true if the two formats are identical.
 */
bool VuoImageFormat_areEqual(const VuoImageFormat value1, const VuoImageFormat value2)
{
	return value1 == value2;
}

/**
 * Returns true if value1 < value2.
 */
bool VuoImageFormat_isLessThan(const VuoImageFormat value1, const VuoImageFormat value2)
{
	return value1 < value2;
}
