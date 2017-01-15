/**
 * @file
 * VuoCursor implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoCursor.h"
#include "VuoList_VuoCursor.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Cursor",
					  "description" : "A mouse cursor.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoCursor"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "pointer"
 * }
 */
VuoCursor VuoCursor_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoCursor value = VuoCursor_None;

	if (strcmp(valueAsString, "pointer") == 0)
		value = VuoCursor_Pointer;
	else if (strcmp(valueAsString, "crosshair") == 0)
		value = VuoCursor_Crosshair;
	else if (strcmp(valueAsString, "hand-open") == 0)
		value = VuoCursor_HandOpen;
	else if (strcmp(valueAsString, "hand-closed") == 0)
		value = VuoCursor_HandClosed;
	else if (strcmp(valueAsString, "i-beam") == 0)
		value = VuoCursor_IBeam;
	else if (strcmp(valueAsString, "circle") == 0)
		value = VuoCursor_Circle;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoCursor_getJson(const VuoCursor value)
{
	char *valueAsString = "none";

	if (value == VuoCursor_Pointer)
		valueAsString = "pointer";
	else if (value == VuoCursor_Crosshair)
		valueAsString = "crosshair";
	else if (value == VuoCursor_HandOpen)
		valueAsString = "hand-open";
	else if (value == VuoCursor_HandClosed)
		valueAsString = "hand-closed";
	else if (value == VuoCursor_IBeam)
		valueAsString = "i-beam";
	else if (value == VuoCursor_Circle)
		valueAsString = "circle";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoCursor VuoCursor_getAllowedValues(void)
{
	VuoList_VuoCursor l = VuoListCreate_VuoCursor();
	VuoListAppendValue_VuoCursor(l, VuoCursor_None);
	VuoListAppendValue_VuoCursor(l, VuoCursor_Pointer);
	VuoListAppendValue_VuoCursor(l, VuoCursor_Crosshair);
	VuoListAppendValue_VuoCursor(l, VuoCursor_HandOpen);
	VuoListAppendValue_VuoCursor(l, VuoCursor_HandClosed);
	VuoListAppendValue_VuoCursor(l, VuoCursor_IBeam);
	VuoListAppendValue_VuoCursor(l, VuoCursor_Circle);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoCursor_getSummary(const VuoCursor value)
{
	char *valueAsString = "(no cursor)";

	if (value == VuoCursor_Pointer)
		valueAsString = "Pointer";
	else if (value == VuoCursor_Crosshair)
		valueAsString = "Crosshair";
	else if (value == VuoCursor_HandOpen)
		valueAsString = "Hand (open)";
	else if (value == VuoCursor_HandClosed)
		valueAsString = "Hand (closed)";
	else if (value == VuoCursor_IBeam)
		valueAsString = "I-beam";
	else if (value == VuoCursor_Circle)
		valueAsString = "Circle";

	return strdup(valueAsString);
}

/**
 * Returns true if the cursor is anything other than `none`.
 */
bool VuoCursor_isPopulated(const VuoCursor value)
{
	return (value != VuoCursor_None);
}
