/**
 * @file
 * VuoDeinterlacing implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoDeinterlacing.h"
#include "VuoList_VuoDeinterlacing.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Deinterlacing",
	"description" : "How to convert interlaced video to progressive video.",
	"keywords" : [ ],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoList_VuoDeinterlacing"
	]
});
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "double-alt-smooth"
 * }
 */
VuoDeinterlacing VuoDeinterlacing_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoDeinterlacing value = VuoDeinterlacing_None;

	if (strcmp(valueAsString, "alternate") == 0)
		value = VuoDeinterlacing_AlternateFields;
	else if (strcmp(valueAsString, "blend") == 0)
		value = VuoDeinterlacing_BlendFields;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoDeinterlacing_getJson(const VuoDeinterlacing value)
{
	const char *valueAsString = "none";

	if (value == VuoDeinterlacing_AlternateFields)
		valueAsString = "alternate";
	else if (value == VuoDeinterlacing_BlendFields)
		valueAsString = "blend";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoDeinterlacing VuoDeinterlacing_getAllowedValues(void)
{
	VuoList_VuoDeinterlacing l = VuoListCreate_VuoDeinterlacing();
	VuoListAppendValue_VuoDeinterlacing(l, VuoDeinterlacing_None);
	VuoListAppendValue_VuoDeinterlacing(l, VuoDeinterlacing_AlternateFields);
	VuoListAppendValue_VuoDeinterlacing(l, VuoDeinterlacing_BlendFields);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoDeinterlacing_getSummary(const VuoDeinterlacing value)
{
	const char *valueAsString = "None";

	if (value == VuoDeinterlacing_AlternateFields)
		valueAsString = "Alternate fields";
	else if (value == VuoDeinterlacing_BlendFields)
		valueAsString = "Blend fields";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoDeinterlacing_areEqual(const VuoDeinterlacing valueA, const VuoDeinterlacing valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoDeinterlacing_isLessThan(const VuoDeinterlacing valueA, const VuoDeinterlacing valueB)
{
	return valueA < valueB;
}
