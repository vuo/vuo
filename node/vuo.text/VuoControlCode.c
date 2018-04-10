/**
 * @file
 * VuoControlCode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoControlCode.h"
#include "VuoList_VuoControlCode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Control Code",
					  "description" : "Special characters like spaces, returns, and tab.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoText",
					  "VuoList_VuoControlCode"
					  ]
				  });
#endif
/// @}

/**
 * Creates a new VuoText string from a control code.
 */
VuoText VuoControlCode_makeText(VuoControlCode code)
{
	if( code == VuoControlCode_NewLineUnix )
		return VuoText_make("\n");
	else if( code == VuoControlCode_NewLineWindows )
		return VuoText_make("\r\n");
	else if( code == VuoControlCode_NewLineMacOS9 )
		return VuoText_make("\r");
	else if( code == VuoControlCode_Tab )
		return VuoText_make("\t");
	else if( code == VuoControlCode_Space )
		return VuoText_make(" ");
	else if( code == VuoControlCode_EmSpace )
		return VuoText_make("\u2003");
	else if( code == VuoControlCode_EnSpace )
		return VuoText_make("\u2002");

	return VuoText_make("");
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "percentage"
 * }
 */
VuoControlCode VuoControlCode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoControlCode value = VuoControlCode_NewLineUnix;

	if(strcmp(valueAsString, "newline-unix") == 0)
		value = VuoControlCode_NewLineUnix;
	else if(strcmp(valueAsString, "newline-windows") == 0)
		value = VuoControlCode_NewLineWindows;
	else if(strcmp(valueAsString, "newline-macos9") == 0)
		value = VuoControlCode_NewLineMacOS9;
	else if(strcmp(valueAsString, "tab") == 0)
		value = VuoControlCode_Tab;
	else if(strcmp(valueAsString, "space") == 0)
		value = VuoControlCode_Space;
	else if(strcmp(valueAsString, "em-space") == 0)
		value = VuoControlCode_EmSpace;
	else if(strcmp(valueAsString, "en-space") == 0)
		value = VuoControlCode_EnSpace;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoControlCode_getJson(const VuoControlCode value)
{
	char *valueAsString = "newline-unix";

	if(value == VuoControlCode_NewLineUnix)
		valueAsString = "newline-unix";
	else if(value == VuoControlCode_NewLineWindows)
		valueAsString = "newline-windows";
	else if(value == VuoControlCode_NewLineMacOS9)
		valueAsString = "newline-macos9";
	else if(value == VuoControlCode_Tab)
		valueAsString = "tab";
	else if(value == VuoControlCode_Space)
		valueAsString = "space";
	else if(value == VuoControlCode_EmSpace)
		valueAsString = "em-space";
	else if(value == VuoControlCode_EnSpace)
		valueAsString = "en-space";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoControlCode VuoControlCode_getAllowedValues(void)
{
	VuoList_VuoControlCode l = VuoListCreate_VuoControlCode();

	VuoListAppendValue_VuoControlCode(l, VuoControlCode_NewLineUnix);
	VuoListAppendValue_VuoControlCode(l, VuoControlCode_NewLineWindows);
	VuoListAppendValue_VuoControlCode(l, VuoControlCode_NewLineMacOS9);

	// Sorted by ascending width.
	VuoListAppendValue_VuoControlCode(l, VuoControlCode_Space);
	VuoListAppendValue_VuoControlCode(l, VuoControlCode_EnSpace);
	VuoListAppendValue_VuoControlCode(l, VuoControlCode_EmSpace);
	VuoListAppendValue_VuoControlCode(l, VuoControlCode_Tab);

	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoControlCode_getSummary(const VuoControlCode value)
{
	char *valueAsString = "New Line (Unix + macOS)";

	if(value == VuoControlCode_NewLineWindows)
		valueAsString = "New Line (Windows)";
	else if(value == VuoControlCode_NewLineMacOS9)
		valueAsString = "New Line (Mac OS 9)";
	else if(value == VuoControlCode_Tab)
		valueAsString = "Tab";
	else if(value == VuoControlCode_Space)
		valueAsString = "Space";
	else if(value == VuoControlCode_EmSpace)
		valueAsString = "Em Space";
	else if(value == VuoControlCode_EnSpace)
		valueAsString = "En Space";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoControlCode_areEqual(const VuoControlCode valueA, const VuoControlCode valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoControlCode_isLessThan(const VuoControlCode valueA, const VuoControlCode valueB)
{
	return valueA < valueB;
}

