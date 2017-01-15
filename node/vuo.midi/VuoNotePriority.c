/**
 * @file
 * VuoNotePriority implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoNotePriority.h"
#include "VuoList_VuoNotePriority.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Note Priority",
					  "description" : "Specifies the algorithm for collapsing multiple simultaneously-pressed notes into a single note.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "c"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "last"
 * }
 */
VuoNotePriority VuoNotePriority_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoNotePriority value = VuoNotePriority_First;

	if (strcmp(valueAsString, "last") == 0)
		value = VuoNotePriority_Last;
	else if (strcmp(valueAsString, "lowest") == 0)
		value = VuoNotePriority_Lowest;
	else if (strcmp(valueAsString, "highest") == 0)
		value = VuoNotePriority_Highest;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoNotePriority_getJson(const VuoNotePriority value)
{
	char *valueAsString = "first";

	if (value == VuoNotePriority_Last)
		valueAsString = "last";
	else if (value == VuoNotePriority_Lowest)
		valueAsString = "lowest";
	else if (value == VuoNotePriority_Highest)
		valueAsString = "highest";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoNotePriority VuoNotePriority_getAllowedValues(void)
{
	VuoList_VuoNotePriority l = VuoListCreate_VuoNotePriority();
	VuoListAppendValue_VuoNotePriority(l, VuoNotePriority_First);
	VuoListAppendValue_VuoNotePriority(l, VuoNotePriority_Last);
	VuoListAppendValue_VuoNotePriority(l, VuoNotePriority_Lowest);
	VuoListAppendValue_VuoNotePriority(l, VuoNotePriority_Highest);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoNotePriority_getSummary(const VuoNotePriority value)
{
	char *valueAsString = "First Note";

	if (value == VuoNotePriority_Last)
		valueAsString = "Last Note";
	else if (value == VuoNotePriority_Lowest)
		valueAsString = "Lowest Note";
	else if (value == VuoNotePriority_Highest)
		valueAsString = "Highest Note";

	return strdup(valueAsString);
}
