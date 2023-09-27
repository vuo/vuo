/**
 * @file
 * VuoPixelShape implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPixelShape.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Pixel Shape",
					  "description" : "The shape to use for enlarged pixels.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoPixelShape"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "triangle"
 * }
 */
VuoPixelShape VuoPixelShape_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoPixelShape value = VuoPixelShape_Rectangle;

	if (strcmp(valueAsString, "triangle") == 0)
		value = VuoPixelShape_Triangle;
	else if (strcmp(valueAsString, "hexagon") == 0)
		value = VuoPixelShape_Hexagon;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoPixelShape_getJson(const VuoPixelShape value)
{
	char *valueAsString = "rectangle";

	if (value == VuoPixelShape_Triangle)
		valueAsString = "triangle";
	else if (value == VuoPixelShape_Hexagon)
		valueAsString = "hexagon";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoPixelShape VuoPixelShape_getAllowedValues(void)
{
	VuoList_VuoPixelShape l = VuoListCreate_VuoPixelShape();
	VuoListAppendValue_VuoPixelShape(l, VuoPixelShape_Rectangle);
	VuoListAppendValue_VuoPixelShape(l, VuoPixelShape_Triangle);
	VuoListAppendValue_VuoPixelShape(l, VuoPixelShape_Hexagon);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoPixelShape_getSummary(const VuoPixelShape value)
{
	char *valueAsString = "Rectangle";

	if (value == VuoPixelShape_Triangle)
		valueAsString = "Triangle";
	else if (value == VuoPixelShape_Hexagon)
		valueAsString = "Hexagon";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoPixelShape_areEqual(const VuoPixelShape valueA, const VuoPixelShape valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoPixelShape_isLessThan(const VuoPixelShape valueA, const VuoPixelShape valueB)
{
	return valueA < valueB;
}
