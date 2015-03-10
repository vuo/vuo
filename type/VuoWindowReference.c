/**
 * @file
 * VuoWindowReference implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include "type.h"
#include "VuoWindowReference.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Window",
					 "version" : "1.0.0"
				 });
#endif
/// @}

/**
 * @ingroup VuoWindowReference
 * Creates a VuoWindowReference from a VuoWindow. Since the VuoWindowReference contains the memory address
 * of the VuoWindow, it's only valid as long as the VuoWindow remains in memory.
 */
VuoWindowReference VuoWindowReference_make(void *window)
{
	return (VuoWindowReference)window;
}

/**
 * @ingroup VuoWindowReference
 * Decodes the JSON object @a js, expected to contain a string, to create a new VuoMouseButton.
 */
VuoWindowReference VuoWindowReference_valueFromJson(json_object * js)
{
	return json_object_get_int64(js);
}

/**
 * @ingroup VuoWindowReference
 * Encodes @a value as a JSON object.
 */
json_object * VuoWindowReference_jsonFromValue(const VuoWindowReference value)
{
	return json_object_new_int64(value);
}

/**
 * @ingroup VuoWindowReference
 * Returns a brief human-readable summary of @a value.
 */
char * VuoWindowReference_summaryFromValue(const VuoWindowReference value)
{
	if (value == 0)
		return strdup("(no window)");

	json_object *js = VuoWindowReference_jsonFromValue(value);
	char *summary = strdup(json_object_to_json_string_ext(js,JSON_C_TO_STRING_PLAIN));
	json_object_put(js);
	return summary;
}
