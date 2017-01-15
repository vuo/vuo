/**
 * @file
 * VuoListPosition implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoListPosition.h"
#include "VuoList_VuoListPosition.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "List Position",
					  "description" : "A position in a list.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoListPosition"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "end"
 * }
 */
VuoListPosition VuoListPosition_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoListPosition value = VuoListPosition_Beginning;

	if (strcmp(valueAsString, "end") == 0)
		value = VuoListPosition_End;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoListPosition_getJson(const VuoListPosition value)
{
	char *valueAsString = "beginning";

	if (value == VuoListPosition_End)
		valueAsString = "end";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoListPosition VuoListPosition_getAllowedValues(void)
{
	VuoList_VuoListPosition l = VuoListCreate_VuoListPosition();
	VuoListAppendValue_VuoListPosition(l, VuoListPosition_Beginning);
	VuoListAppendValue_VuoListPosition(l, VuoListPosition_End);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoListPosition_getSummary(const VuoListPosition value)
{
	char *valueAsString = "Beginning";

	if (value == VuoListPosition_End)
		valueAsString = "End";

	return strdup(valueAsString);
}
