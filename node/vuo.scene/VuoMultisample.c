/**
 * @file
 * VuoMultisample implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMultisample.h"
#include "VuoList_VuoMultisample.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Multisampling",
					  "description" : "Number of samples per pixel.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoMultisample"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "2"
 * }
 */
VuoMultisample VuoMultisample_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoMultisample value = VuoMultisample_Off;

	if (strcmp(valueAsString, "2") == 0)
		value = VuoMultisample_2;
	else if (strcmp(valueAsString, "4") == 0)
		value = VuoMultisample_4;
	else if (strcmp(valueAsString, "8") == 0)
		value = VuoMultisample_8;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoMultisample_getJson(const VuoMultisample value)
{
	char *valueAsString = "off";

	if (value == VuoMultisample_2)
		valueAsString = "2";
	else if (value == VuoMultisample_4)
		valueAsString = "4";
	else if (value == VuoMultisample_8)
		valueAsString = "8";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoMultisample VuoMultisample_getAllowedValues(void)
{
	VuoList_VuoMultisample l = VuoListCreate_VuoMultisample();
	VuoListAppendValue_VuoMultisample(l, VuoMultisample_Off);
	VuoListAppendValue_VuoMultisample(l, VuoMultisample_2);
	VuoListAppendValue_VuoMultisample(l, VuoMultisample_4);
	VuoListAppendValue_VuoMultisample(l, VuoMultisample_8);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoMultisample_getSummary(const VuoMultisample value)
{
	char *valueAsString = "Off";

	if (value == VuoMultisample_2)
		valueAsString = "2x";
	else if (value == VuoMultisample_4)
		valueAsString = "4x";
	else if (value == VuoMultisample_8)
		valueAsString = "8x";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoMultisample_areEqual(const VuoMultisample valueA, const VuoMultisample valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoMultisample_isLessThan(const VuoMultisample valueA, const VuoMultisample valueB)
{
	return valueA < valueB;
}

