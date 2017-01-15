/**
 * @file
 * VuoVerticalReflection implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoVerticalReflection.h"
#include "VuoList_VuoVerticalReflection.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Vertical Reflection",
					 "description" : "Options for mirroring an image vertically",
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoVerticalReflection"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoVerticalReflection
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoVerticalReflection.
 */
VuoVerticalReflection VuoVerticalReflection_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoVerticalReflection value = VuoVerticalReflection_None;

	if( !strcmp(valueAsString, "top"))
		value = VuoVerticalReflection_Top;
	else if (!strcmp(valueAsString, "bottom"))
		value = VuoVerticalReflection_Bottom;

	return value;
}

/**
 * @ingroup VuoVerticalReflection
 * Encodes @c value as a JSON object.
 */
json_object * VuoVerticalReflection_getJson(const VuoVerticalReflection value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoVerticalReflection_None:
			valueAsString = "none";
			break;

		case VuoVerticalReflection_Top:
			valueAsString = "top";
			break;

		case VuoVerticalReflection_Bottom:
			valueAsString = "bottom";
			break;
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoVerticalReflection VuoVerticalReflection_getAllowedValues(void)
{
	VuoList_VuoVerticalReflection l = VuoListCreate_VuoVerticalReflection();
	VuoListAppendValue_VuoVerticalReflection(l, VuoVerticalReflection_None);
	VuoListAppendValue_VuoVerticalReflection(l, VuoVerticalReflection_Top);
	VuoListAppendValue_VuoVerticalReflection(l, VuoVerticalReflection_Bottom);
	return l;
}
/**
 * @ingroup VuoVerticalReflection
 * Returns a human-readable description of @a value.
 */
char * VuoVerticalReflection_getSummary(const VuoVerticalReflection value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoVerticalReflection_None:
			valueAsString = "None";
			break;

		case VuoVerticalReflection_Top:
			valueAsString = "Reflect Top Half";
			break;

		case VuoVerticalReflection_Bottom:
			valueAsString = "Reflect Bottom Half";
			break;
	}

	return strdup(valueAsString);
}
