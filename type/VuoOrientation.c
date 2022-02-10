/**
 * @file
 * VuoOrientation implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Orientation",
					  "description" : "Horizontal or vertical alignment.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoOrientation"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoOrientation
 * Decodes the JSON object @c js to create a new value.
 *
 * @version200New
 */
VuoOrientation VuoOrientation_makeFromJson(json_object * js)
{
	const char *valueAsString = "";

	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "vertical") == 0)
		return VuoOrientation_Vertical;

	return VuoOrientation_Horizontal;
}

/**
 * @ingroup VuoOrientation
 * Encodes @c value as a JSON object.
 *
 * @version200New
 */
json_object * VuoOrientation_getJson(const VuoOrientation value)
{
	char *valueAsString = "horizontal";

	if (value == VuoOrientation_Vertical)
		valueAsString = "vertical";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 *
 * @version200New
 */
VuoList_VuoOrientation VuoOrientation_getAllowedValues(void)
{
	VuoList_VuoOrientation l = VuoListCreate_VuoOrientation();
	VuoListAppendValue_VuoOrientation(l, VuoOrientation_Horizontal);
	VuoListAppendValue_VuoOrientation(l, VuoOrientation_Vertical);
	return l;
}

/**
 * @ingroup VuoOrientation
 * Returns a compact string representation of @c value.
 *
 * @version200New
 */
char * VuoOrientation_getSummary(const VuoOrientation value)
{
	char *valueAsString = "Horizontal";

	if (value == VuoOrientation_Vertical)
		valueAsString = "Vertical";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 *
 * @version200New
 */
bool VuoOrientation_areEqual(const VuoOrientation valueA, const VuoOrientation valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 *
 * @version200New
 */
bool VuoOrientation_isLessThan(const VuoOrientation valueA, const VuoOrientation valueB)
{
	return valueA < valueB;
}
