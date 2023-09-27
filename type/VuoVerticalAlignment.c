/**
 * @file
 * VuoVerticalAlignment implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "VuoVerticalAlignment.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Vertical Alignment",
					  "description" : "Vertical alignment.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoVerticalAlignment"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoVerticalAlignment
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "replaceThis" : -1
 *   }
 * }
 */
VuoVerticalAlignment VuoVerticalAlignment_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "center") == 0)
		return VuoVerticalAlignment_Center;
	else if (strcmp(valueAsString, "bottom") == 0)
		return VuoVerticalAlignment_Bottom;

	return VuoVerticalAlignment_Top;
}

/**
 * @ingroup VuoVerticalAlignment
 * Encodes @c value as a JSON object.
 */
json_object * VuoVerticalAlignment_getJson(const VuoVerticalAlignment value)
{
	char *valueAsString = "top";

	if (value == VuoVerticalAlignment_Center)
		valueAsString = "center";
	else if (value == VuoVerticalAlignment_Bottom)
		valueAsString = "bottom";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoVerticalAlignment VuoVerticalAlignment_getAllowedValues(void)
{
	VuoList_VuoVerticalAlignment l = VuoListCreate_VuoVerticalAlignment();
	VuoListAppendValue_VuoVerticalAlignment(l, VuoVerticalAlignment_Top);
	VuoListAppendValue_VuoVerticalAlignment(l, VuoVerticalAlignment_Center);
	VuoListAppendValue_VuoVerticalAlignment(l, VuoVerticalAlignment_Bottom);
	return l;
}

/**
 * @ingroup VuoVerticalAlignment
 * Returns a compact string representation of @c value.
 */
char * VuoVerticalAlignment_getSummary(const VuoVerticalAlignment value)
{
	char *valueAsString = "Top";

	if (value == VuoVerticalAlignment_Center)
		valueAsString = "Center";
	else if (value == VuoVerticalAlignment_Bottom)
		valueAsString = "Bottom";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoVerticalAlignment_areEqual(const VuoVerticalAlignment valueA, const VuoVerticalAlignment valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoVerticalAlignment_isLessThan(const VuoVerticalAlignment valueA, const VuoVerticalAlignment valueB)
{
	return valueA < valueB;
}
