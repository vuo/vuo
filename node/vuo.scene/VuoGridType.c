/**
 * @file
 * VuoGridType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoGridType.h"
#include "VuoList_VuoGridType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Grid Type",
					  "description" : "Defines the different types of grids.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoGridType"
					 ]
				  });
#endif
/// @}

/**
 * @ingroup VuoGridType
 * Decodes the JSON object @c js to create a new value.
 */
VuoGridType VuoGridType_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoGridType value = VuoGridType_Horizontal;

	if (!strcmp(valueAsString, "vertical")) {
		value = VuoGridType_Vertical;
	} else if (!strcmp(valueAsString, "horizontal-vertical")) {
		value = VuoGridType_HorizontalAndVertical;
	}

	return value;
}

/**
 * @ingroup VuoGridType
 * Encodes @c value as a JSON object.
 */
json_object * VuoGridType_getJson(const VuoGridType value)
{
	char *valueAsString = "horizontal";

	if (value == VuoGridType_Vertical) {
		valueAsString = "vertical";
	} else if(value == VuoGridType_HorizontalAndVertical) {
		valueAsString = "horizontal-vertical";
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoGridType VuoGridType_getAllowedValues(void)
{
	VuoList_VuoGridType l = VuoListCreate_VuoGridType();
	VuoListAppendValue_VuoGridType(l, VuoGridType_Horizontal);
	VuoListAppendValue_VuoGridType(l, VuoGridType_Vertical);
	VuoListAppendValue_VuoGridType(l, VuoGridType_HorizontalAndVertical);
	return l;
}

/**
 * @ingroup VuoGridType
 * Returns a compact string representation of @c value.
 */
char * VuoGridType_getSummary(const VuoGridType value)
{
	char *valueAsString = "Horizontal";

	if(value == VuoGridType_Vertical) {
		valueAsString = "Vertical";
	} else if(value == VuoGridType_HorizontalAndVertical) {
		valueAsString = "Horizontal and Vertical";
	}

	return strdup(valueAsString);
}
