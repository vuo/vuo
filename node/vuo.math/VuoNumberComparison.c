/**
 * @file
 * VuoNumberComparison implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoNumberComparison.h"
#include "VuoList_VuoNumberComparison.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Number Comparison",
					  "description" : "How to compare two numbers.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoNumberComparison"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "percentage"
 * }
 */
VuoNumberComparison VuoNumberComparison_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoNumberComparison value = VuoNumberComparison_Equal;

	if (strcmp(valueAsString, "≠") == 0)
		value = VuoNumberComparison_NotEqual;
	else if (strcmp(valueAsString, "<") == 0)
		value = VuoNumberComparison_LessThan;
	else if (strcmp(valueAsString, "≤") == 0)
		value = VuoNumberComparison_LessThanOrEqual;
	else if (strcmp(valueAsString, ">") == 0)
		value = VuoNumberComparison_GreaterThan;
	else if (strcmp(valueAsString, "≥") == 0)
		value = VuoNumberComparison_GreaterThanOrEqual;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoNumberComparison_getJson(const VuoNumberComparison value)
{
	char *valueAsString = "=";

	if (value == VuoNumberComparison_NotEqual)
		valueAsString = "≠";
	else if (value == VuoNumberComparison_LessThan)
		valueAsString = "<";
	else if (value == VuoNumberComparison_LessThanOrEqual)
		valueAsString = "≤";
	else if (value == VuoNumberComparison_GreaterThan)
		valueAsString = ">";
	else if (value == VuoNumberComparison_GreaterThanOrEqual)
		valueAsString = "≥";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoNumberComparison VuoNumberComparison_getAllowedValues(void)
{
	VuoList_VuoNumberComparison l = VuoListCreate_VuoNumberComparison();
	VuoListAppendValue_VuoNumberComparison(l, VuoNumberComparison_Equal);
	VuoListAppendValue_VuoNumberComparison(l, VuoNumberComparison_NotEqual);
	VuoListAppendValue_VuoNumberComparison(l, VuoNumberComparison_LessThan);
	VuoListAppendValue_VuoNumberComparison(l, VuoNumberComparison_LessThanOrEqual);
	VuoListAppendValue_VuoNumberComparison(l, VuoNumberComparison_GreaterThan);
	VuoListAppendValue_VuoNumberComparison(l, VuoNumberComparison_GreaterThanOrEqual);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoNumberComparison_getSummary(const VuoNumberComparison value)
{
	char *valueAsString = "=";

	if (value == VuoNumberComparison_NotEqual)
		valueAsString = "≠";
	else if (value == VuoNumberComparison_LessThan)
		valueAsString = "<";
	else if (value == VuoNumberComparison_LessThanOrEqual)
		valueAsString = "≤";
	else if (value == VuoNumberComparison_GreaterThan)
		valueAsString = ">";
	else if (value == VuoNumberComparison_GreaterThanOrEqual)
		valueAsString = "≥";

	return strdup(valueAsString);
}
