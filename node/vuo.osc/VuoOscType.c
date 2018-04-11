/**
 * @file
 * VuoOscType implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoOscType.h"
#include "VuoList_VuoOscType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "OSC Type",
					  "description" : "An OSC data type.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoOscType"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "int32"
 * }
 */
VuoOscType VuoOscType_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoOscType value = VuoOscType_Auto;

	if (strcmp(valueAsString, "int32") == 0)
		value = VuoOscType_Int32;
	else if (strcmp(valueAsString, "float32") == 0)
		value = VuoOscType_Float32;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoOscType_getJson(const VuoOscType value)
{
	char *valueAsString = "auto";

	if (value == VuoOscType_Int32)
		valueAsString = "int32";
	else if (value == VuoOscType_Float32)
		valueAsString = "float32";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoOscType VuoOscType_getAllowedValues(void)
{
	VuoList_VuoOscType l = VuoListCreate_VuoOscType();
	VuoListAppendValue_VuoOscType(l, VuoOscType_Auto);
	VuoListAppendValue_VuoOscType(l, VuoOscType_Int32);
	VuoListAppendValue_VuoOscType(l, VuoOscType_Float32);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoOscType_getSummary(const VuoOscType value)
{
	char *valueAsString = "(auto)";

	if (value == VuoOscType_Int32)
		valueAsString = "Integer (32-bit)";
	else if (value == VuoOscType_Float32)
		valueAsString = "Floating point (32-bit)";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoOscType_areEqual(const VuoOscType valueA, const VuoOscType valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoOscType_isLessThan(const VuoOscType valueA, const VuoOscType valueB)
{
	return valueA < valueB;
}
