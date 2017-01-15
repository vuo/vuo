/**
 * @file
 * VuoIconPosition implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoIconPosition.h"
#include "VuoList_VuoIconPosition.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Icon Position",
					  "description" : "The position of an icon image relative to its label",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoIconPosition"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "right"
 * }
 */
VuoIconPosition VuoIconPosition_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoIconPosition value = VuoIconPosition_Left;

	if (strcmp(valueAsString, "right") == 0)
		value = VuoIconPosition_Right;
	else if (strcmp(valueAsString, "above") == 0)
		value = VuoIconPosition_Above;
	else if (strcmp(valueAsString, "below") == 0)
		value = VuoIconPosition_Below;
	else if (strcmp(valueAsString, "behind") == 0)
		value = VuoIconPosition_Behind;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoIconPosition_getJson(const VuoIconPosition value)
{
	char *valueAsString = "left";

	if (value == VuoIconPosition_Right)
		valueAsString = "right";
	else if (value == VuoIconPosition_Above)
		valueAsString = "above";
	else if (value == VuoIconPosition_Below)
		valueAsString = "below";
	else if (value == VuoIconPosition_Behind)
		valueAsString = "behind";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoIconPosition VuoIconPosition_getAllowedValues(void)
{
	VuoList_VuoIconPosition l = VuoListCreate_VuoIconPosition();
	VuoListAppendValue_VuoIconPosition(l, VuoIconPosition_Left);
	VuoListAppendValue_VuoIconPosition(l, VuoIconPosition_Right);
	VuoListAppendValue_VuoIconPosition(l, VuoIconPosition_Above);
	VuoListAppendValue_VuoIconPosition(l, VuoIconPosition_Below);
	VuoListAppendValue_VuoIconPosition(l, VuoIconPosition_Behind);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoIconPosition_getSummary(const VuoIconPosition value)
{
	char *valueAsString = "Left of label";

	if (value == VuoIconPosition_Right)
		valueAsString = "Right of label";
	else if (value == VuoIconPosition_Above)
		valueAsString = "Above label";
	else if (value == VuoIconPosition_Below)
		valueAsString = "Below label";
	else if (value == VuoIconPosition_Behind)
		valueAsString = "Behind label";

	return strdup(valueAsString);
}

/**
 * Returns true if both values are equal.
 */
bool VuoIconPosition_areEqual(const VuoIconPosition value1, const VuoIconPosition value2)
{
	return value1 == value2;
}
