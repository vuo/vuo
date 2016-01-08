/**
 * @file
 * VuoImageFormat implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoImageFormat.h"
#include "VuoList_VuoImageFormat.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Image Format",
					 "description" : "Available formats when exporting images from Vuo.",
					 "keywords" : [ "" ],
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
	}

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
	}

	return strdup(valueAsString);
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
	}

	return strdup(valueAsString);
}
