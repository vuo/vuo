/**
 * @file
 * VuoSizingMode implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSizingMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Sizing Mode",
					 "description" : "Controls how an image is resized in the event that the dimensions are not evenly scaled.",
					 "keywords" : [ "resize", "scale", "fit", "fill", "stretch" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoSizingMode"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoSizingMode
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoSizingMode.
 */
VuoSizingMode VuoSizingMode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "fit") == 0)
		return VuoSizingMode_Fit;
	else if (strcmp(valueAsString, "fill") == 0)
		return VuoSizingMode_Fill;
	else if (strcmp(valueAsString, "proportional") == 0)
		return VuoSizingMode_Proportional;
	else
		return VuoSizingMode_Stretch;
}

/**
 * @ingroup VuoSizingMode
 * Encodes @c value as a JSON object.
 */
json_object * VuoSizingMode_getJson(const VuoSizingMode value)
{
	if (value == VuoSizingMode_Fit)
		return json_object_new_string("fit");
	else if (value == VuoSizingMode_Fill)
		return json_object_new_string("fill");
	else if (value == VuoSizingMode_Proportional)
		return json_object_new_string("proportional");
	else
		return json_object_new_string("stretch");
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoSizingMode VuoSizingMode_getAllowedValues(void)
{
	VuoList_VuoSizingMode l = VuoListCreate_VuoSizingMode();
	VuoListAppendValue_VuoSizingMode(l, VuoSizingMode_Stretch);
	VuoListAppendValue_VuoSizingMode(l, VuoSizingMode_Fit);
	VuoListAppendValue_VuoSizingMode(l, VuoSizingMode_Fill);
	VuoListAppendValue_VuoSizingMode(l, VuoSizingMode_Proportional);
	return l;
}

/**
 * @ingroup VuoSizingMode
 * Same as @c %VuoSizingMode_getString() but with some capitilization involved.
 */
char * VuoSizingMode_getSummary(const VuoSizingMode value)
{
	if (value == VuoSizingMode_Fit)
		return strdup("Fit");
	else if (value == VuoSizingMode_Fill)
		return strdup("Fill");
	else if (value == VuoSizingMode_Proportional)
		return strdup("Proportional");
	else
		return strdup("Stretch");
}
