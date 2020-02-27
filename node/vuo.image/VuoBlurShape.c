/**
 * @file
 * VuoBlurShape implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoBlurShape.h"
#include "VuoList_VuoBlurShape.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Blur Shape",
					  "description" : "Weights for pixels in a blur",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoBlurShape"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "box"
 * }
 */
VuoBlurShape VuoBlurShape_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoBlurShape value = VuoBlurShape_Gaussian;

	if (strcmp(valueAsString, "linear") == 0)
		value = VuoBlurShape_Linear;
	else if (strcmp(valueAsString, "box") == 0)
		value = VuoBlurShape_Box;
	else if (strcmp(valueAsString, "disc") == 0)
		value = VuoBlurShape_Disc;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoBlurShape_getJson(const VuoBlurShape value)
{
	char *valueAsString = "gaussian";

	if (value == VuoBlurShape_Linear)
		valueAsString = "linear";
	else if (value == VuoBlurShape_Box)
		valueAsString = "box";
	else if (value == VuoBlurShape_Disc)
		valueAsString = "disc";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoBlurShape VuoBlurShape_getAllowedValues(void)
{
	VuoList_VuoBlurShape l = VuoListCreate_VuoBlurShape();
	VuoListAppendValue_VuoBlurShape(l, VuoBlurShape_Gaussian);
	VuoListAppendValue_VuoBlurShape(l, VuoBlurShape_Linear);
	VuoListAppendValue_VuoBlurShape(l, VuoBlurShape_Box);
	VuoListAppendValue_VuoBlurShape(l, VuoBlurShape_Disc);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoBlurShape_getSummary(const VuoBlurShape value)
{
	char *valueAsString = "Gaussian";

	if (value == VuoBlurShape_Linear)
		valueAsString = "Linear";
	else if (value == VuoBlurShape_Box)
		valueAsString = "Box";
	else if (value == VuoBlurShape_Disc)
		valueAsString = "Disc";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoBlurShape_areEqual(const VuoBlurShape valueA, const VuoBlurShape valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoBlurShape_isLessThan(const VuoBlurShape valueA, const VuoBlurShape valueB)
{
	return valueA < valueB;
}
