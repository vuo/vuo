/**
 * @file
 * VuoRealRegulation implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
VuoRealRegulation VuoRealRegulation_valueFromJson(json_object * js)
{
	VuoRealRegulation value = {"",0,0,0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_valueFromJson(o);
	else
		value.name = VuoText_make("");

	if (json_object_object_get_ex(js, "minimumValue", &o))
		value.minimumValue = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "maximumValue", &o))
		value.maximumValue = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "defaultValue", &o))
		value.defaultValue = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "smoothDuration", &o))
		value.smoothDuration = VuoReal_valueFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoRealRegulation_jsonFromValue(const VuoRealRegulation value)
{
	json_object *js = json_object_new_object();
	json_object_object_add(js, "name", VuoText_jsonFromValue(value.name));
	json_object_object_add(js, "minimumValue", VuoReal_jsonFromValue(value.minimumValue));
	json_object_object_add(js, "maximumValue", VuoReal_jsonFromValue(value.maximumValue));
	json_object_object_add(js, "defaultValue", VuoReal_jsonFromValue(value.defaultValue));
	json_object_object_add(js, "smoothDuration", VuoReal_jsonFromValue(value.smoothDuration));
	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoRealRegulation_summaryFromValue(const VuoRealRegulation value)
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
	value.minimumValue = minimumValue;
	value.maximumValue = maximumValue;
	value.defaultValue = defaultValue;
	value.smoothDuration = smoothDuration;
	return value;
}
