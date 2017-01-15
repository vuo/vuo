/**
 * @file
 * VuoSizingMode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoSizingMode.h"
#include "VuoList_VuoSizingMode.h"

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

	VuoSizingMode value = VuoSizingMode_Stretch;

	if (! strcmp(valueAsString, "stretch")) {
		value = VuoSizingMode_Stretch;
	} else if (! strcmp(valueAsString, "fit")) {
		value = VuoSizingMode_Fit;
	} else if (! strcmp(valueAsString, "fill")) {
		value = VuoSizingMode_Fill;
	}

	return value;
}

/**
 * @ingroup VuoSizingMode
 * Encodes @c value as a JSON object.
 */
json_object * VuoSizingMode_getJson(const VuoSizingMode value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoSizingMode_Stretch:
			valueAsString = "stretch";
			break;
		case VuoSizingMode_Fit:
			valueAsString = "fit";
			break;
		case VuoSizingMode_Fill:
			valueAsString = "fill";
			break;
	}

	return json_object_new_string(valueAsString);
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
	return l;
}

/**
 * @ingroup VuoSizingMode
 * Same as @c %VuoSizingMode_getString() but with some capitilization involved.
 */
char * VuoSizingMode_getSummary(const VuoSizingMode value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoSizingMode_Stretch:
			valueAsString = "Stretch";
			break;
		case VuoSizingMode_Fit:
			valueAsString = "Fit";
			break;
		case VuoSizingMode_Fill:
			valueAsString = "Fill";
			break;
	}

	return strdup(valueAsString);
}
