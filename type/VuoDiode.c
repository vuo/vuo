/**
 * @file
 * VuoDiode implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "VuoDiode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Diode",
					  "description" : "How to manage signal polarity.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoDiode"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "bipolar"
 * }
 */
VuoDiode VuoDiode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoDiode value = VuoDiode_Unipolar;

	if (strcmp(valueAsString, "bipolar") == 0)
		value = VuoDiode_Bipolar;
	else if (strcmp(valueAsString, "absolute") == 0)
		value = VuoDiode_Absolute;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoDiode_getJson(const VuoDiode value)
{
	char *valueAsString = "unipolar";

	if (value == VuoDiode_Bipolar)
		valueAsString = "bipolar";
	else if (value == VuoDiode_Absolute)
		valueAsString = "absolute";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoDiode VuoDiode_getAllowedValues(void)
{
	VuoList_VuoDiode l = VuoListCreate_VuoDiode();
	VuoListAppendValue_VuoDiode(l, VuoDiode_Unipolar);
	VuoListAppendValue_VuoDiode(l, VuoDiode_Bipolar);
	VuoListAppendValue_VuoDiode(l, VuoDiode_Absolute);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoDiode_getSummary(const VuoDiode value)
{
	const char *valueAsString = "Unipolar";

	if (value == VuoDiode_Bipolar)
		valueAsString = "Bipolar";
	else if (value == VuoDiode_Absolute)
		valueAsString = "Absolute";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoDiode_areEqual(const VuoDiode valueA, const VuoDiode valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoDiode_isLessThan(const VuoDiode valueA, const VuoDiode valueB)
{
	return valueA < valueB;
}
