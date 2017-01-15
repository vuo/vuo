/**
 * @file
 * VuoCurveEasing implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoCurveEasing.h"
#include "VuoList_VuoCurveEasing.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Curve Easing",
					 "description" : "Specifies which part of a curve is eased.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoCurveEasing"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoCurveEasing
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoCurveEasing.
 */
VuoCurveEasing VuoCurveEasing_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "out") == 0)
		return VuoCurveEasing_Out;
	else if (strcmp(valueAsString, "in+out") == 0)
		return VuoCurveEasing_InOut;
	else if (strcmp(valueAsString, "middle") == 0)
		return VuoCurveEasing_Middle;

	return VuoCurveEasing_In;
}

/**
 * @ingroup VuoCurveEasing
 * Encodes @c value as a JSON object.
 */
json_object * VuoCurveEasing_getJson(const VuoCurveEasing value)
{
	char *valueAsString = "in";

	if (value == VuoCurveEasing_Out)
		valueAsString = "out";
	else if (value == VuoCurveEasing_InOut)
		valueAsString = "in+out";
	else if (value == VuoCurveEasing_Middle)
		valueAsString = "middle";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoCurveEasing VuoCurveEasing_getAllowedValues(void)
{
	VuoList_VuoCurveEasing l = VuoListCreate_VuoCurveEasing();
	VuoListAppendValue_VuoCurveEasing(l, VuoCurveEasing_In);
	VuoListAppendValue_VuoCurveEasing(l, VuoCurveEasing_Out);
	VuoListAppendValue_VuoCurveEasing(l, VuoCurveEasing_InOut);
	VuoListAppendValue_VuoCurveEasing(l, VuoCurveEasing_Middle);
	return l;
}

/**
 * @ingroup VuoCurveEasing
 * Same as @c %VuoCurveEasing_getString()
 */
char * VuoCurveEasing_getSummary(const VuoCurveEasing value)
{
	char *valueAsString = "In";

	if (value == VuoCurveEasing_Out)
		valueAsString = "Out";
	else if (value == VuoCurveEasing_InOut)
		valueAsString = "In + Out";
	else if (value == VuoCurveEasing_Middle)
		valueAsString = "Middle";

	return strdup(valueAsString);
}
