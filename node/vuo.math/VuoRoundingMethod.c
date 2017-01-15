/**
 * @file
 * VuoRoundingMethod implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoRoundingMethod.h"
#include "VuoList_VuoRoundingMethod.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Rounding Method",
					  "description" : "How to round a number.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoRoundingMethod"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "down"
 * }
 */
VuoRoundingMethod VuoRoundingMethod_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoRoundingMethod value = VuoRoundingMethod_Nearest;

	if (strcmp(valueAsString, "down") == 0)
		value = VuoRoundingMethod_Down;
	else if (strcmp(valueAsString, "up") == 0)
		value = VuoRoundingMethod_Up;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoRoundingMethod_getJson(const VuoRoundingMethod value)
{
	char *valueAsString = "nearest";

	if (value == VuoRoundingMethod_Down)
		valueAsString = "down";
	else if (value == VuoRoundingMethod_Up)
		valueAsString = "up";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoRoundingMethod VuoRoundingMethod_getAllowedValues(void)
{
	VuoList_VuoRoundingMethod l = VuoListCreate_VuoRoundingMethod();
	VuoListAppendValue_VuoRoundingMethod(l, VuoRoundingMethod_Nearest);
	VuoListAppendValue_VuoRoundingMethod(l, VuoRoundingMethod_Down);
	VuoListAppendValue_VuoRoundingMethod(l, VuoRoundingMethod_Up);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoRoundingMethod_getSummary(const VuoRoundingMethod value)
{
	char *valueAsString = "Nearest";

	if (value == VuoRoundingMethod_Down)
		valueAsString = "Down";
	else if (value == VuoRoundingMethod_Up)
		valueAsString = "Up";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoRoundingMethod_areEqual(const VuoRoundingMethod valueA, const VuoRoundingMethod valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoRoundingMethod_isLessThan(const VuoRoundingMethod valueA, const VuoRoundingMethod valueB)
{
	return valueA < valueB;
}
