/**
 * @file
 * VuoCoordinateUnit implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "VuoCoordinateUnit.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Coordinate Unit",
					  "description" : "The unit a coordinate uses.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoCoordinateUnit"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "pixels"
 * }
 */
VuoCoordinateUnit VuoCoordinateUnit_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoCoordinateUnit value = VuoCoordinateUnit_Points;

	if (strcmp(valueAsString, "pixels") == 0)
		value = VuoCoordinateUnit_Pixels;
	else if (strcmp(valueAsString, "vuo") == 0)
		value = VuoCoordinateUnit_VuoCoordinates;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoCoordinateUnit_getJson(const VuoCoordinateUnit value)
{
	char *valueAsString = "points";

	if (value == VuoCoordinateUnit_Pixels)
		valueAsString = "pixels";
	else if (value == VuoCoordinateUnit_VuoCoordinates)
		valueAsString = "vuo";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoCoordinateUnit VuoCoordinateUnit_getAllowedValues(void)
{
	VuoList_VuoCoordinateUnit l = VuoListCreate_VuoCoordinateUnit();
	VuoListAppendValue_VuoCoordinateUnit(l, VuoCoordinateUnit_Points);
	VuoListAppendValue_VuoCoordinateUnit(l, VuoCoordinateUnit_Pixels);
	VuoListAppendValue_VuoCoordinateUnit(l, VuoCoordinateUnit_VuoCoordinates);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoCoordinateUnit_getSummary(const VuoCoordinateUnit value)
{
	char *valueAsString = "Points";

	if (value == VuoCoordinateUnit_Pixels)
		valueAsString = "Pixels";
	else if (value == VuoCoordinateUnit_VuoCoordinates)
		valueAsString = "Vuo Coordinates";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoCoordinateUnit_areEqual(const VuoCoordinateUnit valueA, const VuoCoordinateUnit valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoCoordinateUnit_isLessThan(const VuoCoordinateUnit valueA, const VuoCoordinateUnit valueB)
{
	return valueA < valueB;
}
