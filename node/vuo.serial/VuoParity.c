/**
 * @file
 * VuoParity implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoParity.h"
#include "VuoList_VuoParity.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Parity",
					  "description" : "Whether to include an error detection bit, and how to use it.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoParity"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "even"
 * }
 */
VuoParity VuoParity_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoParity value = VuoParity_None;

	if (strcmp(valueAsString, "even") == 0)
		value = VuoParity_Even;
	else if (strcmp(valueAsString, "odd") == 0)
		value = VuoParity_Odd;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoParity_getJson(const VuoParity value)
{
	char *valueAsString = "none";

	if (value == VuoParity_Even)
		valueAsString = "even";
	else if (value == VuoParity_Odd)
		valueAsString = "odd";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoParity VuoParity_getAllowedValues(void)
{
	VuoList_VuoParity l = VuoListCreate_VuoParity();
	VuoListAppendValue_VuoParity(l, VuoParity_None);
	VuoListAppendValue_VuoParity(l, VuoParity_Even);
	VuoListAppendValue_VuoParity(l, VuoParity_Odd);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoParity_getSummary(const VuoParity value)
{
	char *valueAsString = "None";

	if (value == VuoParity_Even)
		valueAsString = "Even";
	else if (value == VuoParity_Odd)
		valueAsString = "Odd";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoParity_areEqual(const VuoParity valueA, const VuoParity valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoParity_isLessThan(const VuoParity valueA, const VuoParity valueB)
{
	return valueA < valueB;
}

