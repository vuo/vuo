/**
 * @file
 * VuoExtrapolationMode implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoExtrapolationMode.h"
#include "VuoList_VuoExtrapolationMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Extrapolation Mode",
					  "description" : "How to extrapolate a list.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoExtrapolationMode"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "stretch"
 * }
 */
VuoExtrapolationMode VuoExtrapolationMode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoExtrapolationMode value = VuoExtrapolationMode_Wrap;

	if (strcmp(valueAsString, "stretch") == 0)
		value = VuoExtrapolationMode_Stretch;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoExtrapolationMode_getJson(const VuoExtrapolationMode value)
{
	char *valueAsString = "wrap";

	if (value == VuoExtrapolationMode_Stretch)
		valueAsString = "stretch";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoExtrapolationMode VuoExtrapolationMode_getAllowedValues(void)
{
	VuoList_VuoExtrapolationMode l = VuoListCreate_VuoExtrapolationMode();
	VuoListAppendValue_VuoExtrapolationMode(l, VuoExtrapolationMode_Wrap);
	VuoListAppendValue_VuoExtrapolationMode(l, VuoExtrapolationMode_Stretch);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoExtrapolationMode_getSummary(const VuoExtrapolationMode value)
{
	char *valueAsString = "Wrap";

	if (value == VuoExtrapolationMode_Stretch)
		valueAsString = "Stretch";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoExtrapolationMode_areEqual(const VuoExtrapolationMode valueA, const VuoExtrapolationMode valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoExtrapolationMode_isLessThan(const VuoExtrapolationMode valueA, const VuoExtrapolationMode valueB)
{
	return valueA < valueB;
}
