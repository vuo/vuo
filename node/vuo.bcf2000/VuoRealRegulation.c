/**
 * @file
 * VuoRealRegulation implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoRealRegulation.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Real Regulation",
					  "description" : "Parameters describing how to regulate a real number.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoReal",
						"VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "name" : "",
 *     "minimumValue" : 0,
 *     "maximumValue" : 1,
 *     "defaultValue" : 0.5,
 *     "smoothDuration" : 2
 *   }
 * }
 */
VuoRealRegulation VuoRealRegulation_makeFromJson(json_object * js)
{
	VuoRealRegulation value = {"",0,0,0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	else
		value.name = VuoText_make("");

	if (json_object_object_get_ex(js, "minimumValue", &o))
		value.minimumValue = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "maximumValue", &o))
		value.maximumValue = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "defaultValue", &o))
		value.defaultValue = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "smoothDuration", &o))
		value.smoothDuration = VuoReal_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoRealRegulation_getJson(const VuoRealRegulation value)
{
	json_object *js = json_object_new_object();
	json_object_object_add(js, "name", VuoText_getJson(value.name));
	json_object_object_add(js, "minimumValue", VuoReal_getJson(value.minimumValue));
	json_object_object_add(js, "maximumValue", VuoReal_getJson(value.maximumValue));
	json_object_object_add(js, "defaultValue", VuoReal_getJson(value.defaultValue));
	json_object_object_add(js, "smoothDuration", VuoReal_getJson(value.smoothDuration));
	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoRealRegulation_getSummary(const VuoRealRegulation value)
{
	return VuoText_format("Ensures %s is between %g and %g, with default %g and smooth duration %g", value.name, value.minimumValue, value.maximumValue, value.defaultValue, value.smoothDuration);
}

/**
 * Returns a Real Regulation with the specified values.
 */
VuoRealRegulation VuoRealRegulation_make(VuoText name, VuoReal minimumValue, VuoReal maximumValue, VuoReal defaultValue, VuoReal smoothDuration)
{
	VuoRealRegulation value;

	value.name = name;

	// Allow the max and min to be in any order (so the user can specify an inverted mapping)…
	value.minimumValue = minimumValue;
	value.maximumValue = maximumValue;

	// …but put them in the right order in order to clamp the default value.
	VuoReal actualMin, actualMax;
	if (minimumValue < maximumValue)
	{
		actualMin = minimumValue;
		actualMax = maximumValue;
	}
	else
	{
		actualMin = maximumValue;
		actualMax = minimumValue;
	}
	value.defaultValue = VuoReal_clamp(defaultValue, actualMin, actualMax);

	value.smoothDuration = smoothDuration;

	return value;
}
