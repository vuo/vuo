/**
 * @file
 * VuoSortOrder implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSortOrder.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Sort Order",
					 "description" : "Ascending or descending order.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoList_VuoSortOrder"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoSortOrder
 * Decodes the JSON object to create a new value.
 */
VuoSortOrder VuoSortOrder_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "descending") == 0)
		return VuoSortOrder_Descending;

	return VuoSortOrder_Ascending;
}

/**
 * @ingroup VuoSortOrder
 * Encodes @a value as a JSON object.
 */
json_object * VuoSortOrder_getJson(const VuoSortOrder value)
{
	char *valueAsString = "ascending";

	if (value == VuoSortOrder_Descending)
		valueAsString = "descending";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoSortOrder VuoSortOrder_getAllowedValues(void)
{
	VuoList_VuoSortOrder l = VuoListCreate_VuoSortOrder();
	VuoListAppendValue_VuoSortOrder(l, VuoSortOrder_Ascending);
	VuoListAppendValue_VuoSortOrder(l, VuoSortOrder_Descending);
	return l;
}

/**
 * @ingroup VuoSortOrder
 * Returns a string representation of @a value.
 */
char * VuoSortOrder_getSummary(const VuoSortOrder value)
{
	char *valueAsString = "Ascending";

	if (value == VuoSortOrder_Descending)
		valueAsString = "Descending";

	return strdup(valueAsString);
}
