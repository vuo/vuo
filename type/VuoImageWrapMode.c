/**
 * @file
 * VuoImageWrapMode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoImageWrapMode.h"
#include "VuoList_VuoImageWrapMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Image Wrap Mode",
					 "description" : "Controls what an image displays when the pixels requested are out of range.",
					 "keywords" : [ "overlap", "repeat", "clamp", "tile" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoImageWrapMode"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoImageWrapMode
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoImageWrapMode.
 */
VuoImageWrapMode VuoImageWrapMode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoImageWrapMode value = VuoImageWrapMode_None;

	if (! strcmp(valueAsString, "none")) {
		value = VuoImageWrapMode_None;
	} else if (! strcmp(valueAsString, "clamp")) {
		value = VuoImageWrapMode_ClampEdge;
	} else if (! strcmp(valueAsString, "repeat")) {
		value = VuoImageWrapMode_Repeat;
	} else if (! strcmp(valueAsString, "mirror")) {
		value = VuoImageWrapMode_MirroredRepeat;
	}

	return value;
}

/**
 * @ingroup VuoImageWrapMode
 * Encodes @c value as a JSON object.
 */
json_object * VuoImageWrapMode_getJson(const VuoImageWrapMode value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoImageWrapMode_None:
			valueAsString = "none";
			break;
		case VuoImageWrapMode_ClampEdge:
			valueAsString = "clamp";
			break;
		case VuoImageWrapMode_Repeat:
			valueAsString = "repeat";
			break;
		case VuoImageWrapMode_MirroredRepeat:
			valueAsString = "mirror";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoImageWrapMode VuoImageWrapMode_getAllowedValues(void)
{
	VuoList_VuoImageWrapMode l = VuoListCreate_VuoImageWrapMode();
	VuoListAppendValue_VuoImageWrapMode(l, VuoImageWrapMode_None);
	VuoListAppendValue_VuoImageWrapMode(l, VuoImageWrapMode_ClampEdge);
	VuoListAppendValue_VuoImageWrapMode(l, VuoImageWrapMode_Repeat);
	VuoListAppendValue_VuoImageWrapMode(l, VuoImageWrapMode_MirroredRepeat);
	return l;
}

/**
 * @ingroup VuoImageWrapMode
 * Same as @c %VuoImageWrapMode_getString()
 */
char * VuoImageWrapMode_getSummary(const VuoImageWrapMode value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoImageWrapMode_None:
			valueAsString = "None";
			break;

		case VuoImageWrapMode_ClampEdge:
			valueAsString = "Clamp Edge";
			break;

		case VuoImageWrapMode_Repeat:
			valueAsString = "Repeat";
			break;

		case VuoImageWrapMode_MirroredRepeat:
			valueAsString = "Mirror Repeat";
			break;
	}
	return strdup(valueAsString);
}
