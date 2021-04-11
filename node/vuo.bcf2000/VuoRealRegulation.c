/**
 * @file
 * VuoRealRegulation implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
	return (VuoRealRegulation){
		VuoJson_getObjectValue(VuoText, js, "name",           NULL),
		VuoJson_getObjectValue(VuoReal, js, "minimumValue",   0),
		VuoJson_getObjectValue(VuoReal, js, "maximumValue",   0),
		VuoJson_getObjectValue(VuoReal, js, "defaultValue",   0),
		VuoJson_getObjectValue(VuoReal, js, "smoothDuration", 0)
	};
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

	value.defaultValue = VuoReal_clamp(defaultValue, minimumValue, maximumValue);

	value.smoothDuration = smoothDuration;

	return value;
}
