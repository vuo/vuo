/**
 * @file
 * VuoBoolean implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Boolean",
					 "description" : "A boolean (true/false) value.",
					 "keywords" : [ "true", "false" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoBoolean"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoBoolean
 * Decodes the JSON object @c js, expected to contain a boolean, to create a new @c VuoBoolean.
 */
VuoBoolean VuoBoolean_makeFromJson(json_object *js)
{
	if (json_object_get_type(js) == json_type_string)
	{
		char firstChar = tolower(json_object_get_string(js)[0]);
		if (firstChar == 'n'  // "no"
		 || firstChar == 'f') // "false"
			return false;
	}

	return json_object_get_boolean(js);
}

/**
 * @ingroup VuoBoolean
 * Encodes @c value as a JSON object.
 */
json_object * VuoBoolean_getJson(const VuoBoolean value)
{
	return json_object_new_boolean(value);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoBoolean VuoBoolean_getAllowedValues(void)
{
	VuoList_VuoBoolean l = VuoListCreate_VuoBoolean();
	VuoListAppendValue_VuoBoolean(l, false);
	VuoListAppendValue_VuoBoolean(l, true);
	return l;
}

/**
 * @ingroup VuoBoolean
 * Always shows the full value, since it's guaranteed to be pretty short.
 */
char * VuoBoolean_getSummary(const VuoBoolean value)
{
	return strdup(value ? "True" : "False");
}

/**
 * Returns true if the two values are equal.
 */
bool VuoBoolean_areEqual(const VuoBoolean value1, const VuoBoolean value2)
{
	return value1 == value2;
}

/**
 * Returns true if `a` is false and `b` is true.
 * @version200New
 */
bool VuoBoolean_isLessThan(const VuoBoolean a, const VuoBoolean b)
{
	return !a && b;
}
