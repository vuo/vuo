/**
 * @file
 * VuoHorizontalReflection implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoHorizontalReflection.h"
#include "VuoList_VuoHorizontalReflection.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Horizontal Reflection",
					 "description" : "Options for mirroring an image horizontally",
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoHorizontalReflection"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoHorizontalReflection
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoHorizontalReflection.
 */
VuoHorizontalReflection VuoHorizontalReflection_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoHorizontalReflection value = VuoHorizontalReflection_None;

	if( !strcmp(valueAsString, "left"))
		value = VuoHorizontalReflection_Left;
	else if (!strcmp(valueAsString, "right"))
		value = VuoHorizontalReflection_Right;

	return value;
}

/**
 * @ingroup VuoHorizontalReflection
 * Encodes @c value as a JSON object.
 */
json_object * VuoHorizontalReflection_getJson(const VuoHorizontalReflection value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoHorizontalReflection_None:
			valueAsString = "none";
			break;

		case VuoHorizontalReflection_Left:
			valueAsString = "left";
			break;

		case VuoHorizontalReflection_Right:
			valueAsString = "right";
			break;
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoHorizontalReflection VuoHorizontalReflection_getAllowedValues(void)
{
	VuoList_VuoHorizontalReflection l = VuoListCreate_VuoHorizontalReflection();
	VuoListAppendValue_VuoHorizontalReflection(l, VuoHorizontalReflection_None);
	VuoListAppendValue_VuoHorizontalReflection(l, VuoHorizontalReflection_Left);
	VuoListAppendValue_VuoHorizontalReflection(l, VuoHorizontalReflection_Right);
	return l;
}
/**
 * @ingroup VuoHorizontalReflection
 * Returns a human-readable description of @a value.
 */
char * VuoHorizontalReflection_getSummary(const VuoHorizontalReflection value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoHorizontalReflection_None:
			valueAsString = "None";
			break;

		case VuoHorizontalReflection_Left:
			valueAsString = "Reflect Left Half";
			break;

		case VuoHorizontalReflection_Right:
			valueAsString = "Reflect Right Half";
			break;
	}

	return strdup(valueAsString);
}
