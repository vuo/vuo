/**
 * @file
 * VuoHorizontalSide implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoHorizontalSide.h"
#include "VuoList_VuoHorizontalSide.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Horizontal Side",
					  "description" : "Direction on the horizontal axis (Right or Left).",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoHorizontalSide",
						"VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{"right"}
 */
VuoHorizontalSide VuoHorizontalSide_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoHorizontalSide value = VuoHorizontalSide_Right;

	if (! strcmp(valueAsString, "left"))
		value = VuoHorizontalSide_Left;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoHorizontalSide_getJson(const VuoHorizontalSide value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoHorizontalSide_Right:
			valueAsString = "right";
			break;
		case VuoHorizontalSide_Left:
			valueAsString = "left";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoHorizontalSide VuoHorizontalSide_getAllowedValues(void)
{
	VuoList_VuoHorizontalSide l = VuoListCreate_VuoHorizontalSide();
	VuoListAppendValue_VuoHorizontalSide(l, VuoHorizontalSide_Left);
	VuoListAppendValue_VuoHorizontalSide(l, VuoHorizontalSide_Right);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoHorizontalSide_getSummary(const VuoHorizontalSide value)
{
	int bits = 0;
	if (value == VuoHorizontalSide_Left)
		return VuoText_format("Left");
	else
		return VuoText_format("Right");

}
