/**
 * @file
 * VuoDisplacement implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoDisplacement.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Displacement",
					  "description" : "The direction in which to move vertices.",
					  "keywords" : [ ],
					  "version" : "1.0.0"
				  });
#endif
/// @}

/**
 * @ingroup VuoDisplacement
 * Decodes the JSON object @c js to create a new value.
 */
VuoDisplacement VuoDisplacement_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoDisplacement value = VuoDisplacement_Transverse;

	if (! strcmp(valueAsString, "longitudinal"))
		value = VuoDisplacement_Longitudinal;

	return value;
}

/**
 * @ingroup VuoDisplacement
 * Encodes @c value as a JSON object.
 */
json_object * VuoDisplacement_getJson(const VuoDisplacement value)
{
	char *valueAsString = "transverse";

	if (value == VuoDisplacement_Longitudinal)
		valueAsString = "longitudinal";

	return json_object_new_string(valueAsString);
}

/**
 * @ingroup VuoDisplacement
 * Returns a compact string representation of @c value.
 */
char * VuoDisplacement_getSummary(const VuoDisplacement value)
{
	char *valueAsString = "Transverse";

	if (value == VuoDisplacement_Longitudinal)
		valueAsString = "Longitudinal";

	return strdup(valueAsString);
}
