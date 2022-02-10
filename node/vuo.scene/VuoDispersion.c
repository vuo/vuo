/**
 * @file
 * VuoDispersion implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoDispersion.h"
#include "VuoList_VuoDispersion.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Dispersion",
					  "description" : "The pattern over which a function is applied.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						 "VuoList_VuoDispersion",
					  ],
				  });
#endif
/// @}

/**
 * @ingroup VuoDispersion
 * Decodes the JSON object @c js to create a new value.
 */
VuoDispersion VuoDispersion_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoDispersion value = VuoDispersion_Linear;

	if (! strcmp(valueAsString, "radial"))
		value = VuoDispersion_Radial;

	return value;
}

/**
 * @ingroup VuoDispersion
 * Encodes @c value as a JSON object.
 */
json_object * VuoDispersion_getJson(const VuoDispersion value)
{
	char *valueAsString = "linear";

	if (value == VuoDispersion_Radial)
		valueAsString = "radial";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 * @version200New
 */
VuoList_VuoDispersion VuoDispersion_getAllowedValues(void)
{
	VuoList_VuoDispersion l = VuoListCreate_VuoDispersion();
	VuoListAppendValue_VuoDispersion(l, VuoDispersion_Linear);
	VuoListAppendValue_VuoDispersion(l, VuoDispersion_Radial);
	return l;
}

/**
 * @ingroup VuoDispersion
 * Returns a compact string representation of @c value.
 */
char * VuoDispersion_getSummary(const VuoDispersion value)
{
	char *valueAsString = "Linear";

	if (value == VuoDispersion_Radial)
		valueAsString = "Radial";

	return strdup(valueAsString);
}
