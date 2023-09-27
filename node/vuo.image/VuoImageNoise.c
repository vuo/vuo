/**
 * @file
 * VuoImageNoise implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageNoise.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Image Noise",
					  "description" : "An image noise pattern.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoImageNoise"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "cellular"
 * }
 */
VuoImageNoise VuoImageNoise_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoImageNoise value = VuoImageNoise_Gradient;

	if (strcmp(valueAsString, "value") == 0)
		value = VuoImageNoise_Value;
	else if (strcmp(valueAsString, "cellular") == 0)
		value = VuoImageNoise_Cellular;
	else if (strcmp(valueAsString, "dot") == 0)
		value = VuoImageNoise_Dot;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoImageNoise_getJson(const VuoImageNoise value)
{
	char *valueAsString = "gradient";

	if (value == VuoImageNoise_Value)
		valueAsString = "value";
	else if (value == VuoImageNoise_Cellular)
		valueAsString = "cellular";
	else if (value == VuoImageNoise_Dot)
		valueAsString = "dot";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoImageNoise VuoImageNoise_getAllowedValues(void)
{
	VuoList_VuoImageNoise l = VuoListCreate_VuoImageNoise();
	VuoListAppendValue_VuoImageNoise(l, VuoImageNoise_Gradient);
	VuoListAppendValue_VuoImageNoise(l, VuoImageNoise_Value);
	VuoListAppendValue_VuoImageNoise(l, VuoImageNoise_Cellular);
	VuoListAppendValue_VuoImageNoise(l, VuoImageNoise_Dot);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoImageNoise_getSummary(const VuoImageNoise value)
{
	char *valueAsString = "Gradient";

	if (value == VuoImageNoise_Value)
		valueAsString = "Value";
	else if (value == VuoImageNoise_Cellular)
		valueAsString = "Cellular";
	else if (value == VuoImageNoise_Dot)
		valueAsString = "Dot";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoImageNoise_areEqual(const VuoImageNoise valueA, const VuoImageNoise valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoImageNoise_isLessThan(const VuoImageNoise valueA, const VuoImageNoise valueB)
{
	return valueA < valueB;
}
