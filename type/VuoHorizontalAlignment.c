/**
 * @file
 * VuoHorizontalAlignment implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoHorizontalAlignment.h"
#include "VuoList_VuoHorizontalAlignment.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Horizontal Alignment",
					  "description" : "Horizontal alignment.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoHorizontalAlignment"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoHorizontalAlignment
 * Decodes the JSON object @c js to create a new value.
 */
VuoHorizontalAlignment VuoHorizontalAlignment_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "center") == 0)
		return VuoHorizontalAlignment_Center;
	else if (strcmp(valueAsString, "right") == 0)
		return VuoHorizontalAlignment_Right;

	return VuoHorizontalAlignment_Left;
}

/**
 * @ingroup VuoHorizontalAlignment
 * Encodes @c value as a JSON object.
 */
json_object * VuoHorizontalAlignment_getJson(const VuoHorizontalAlignment value)
{
	char *valueAsString = "left";

	if (value == VuoHorizontalAlignment_Center)
		valueAsString = "center";
	else if (value == VuoHorizontalAlignment_Right)
		valueAsString = "right";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoHorizontalAlignment VuoHorizontalAlignment_getAllowedValues(void)
{
	VuoList_VuoHorizontalAlignment l = VuoListCreate_VuoHorizontalAlignment();
	VuoListAppendValue_VuoHorizontalAlignment(l, VuoHorizontalAlignment_Left);
	VuoListAppendValue_VuoHorizontalAlignment(l, VuoHorizontalAlignment_Center);
	VuoListAppendValue_VuoHorizontalAlignment(l, VuoHorizontalAlignment_Right);
	return l;
}

/**
 * @ingroup VuoHorizontalAlignment
 * Returns a compact string representation of @c value.
 */
char * VuoHorizontalAlignment_getSummary(const VuoHorizontalAlignment value)
{
	char *valueAsString = "Left";

	if (value == VuoHorizontalAlignment_Center)
		valueAsString = "Center";
	else if (value == VuoHorizontalAlignment_Right)
		valueAsString = "Right";

	return strdup(valueAsString);
}
