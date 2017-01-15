/**
 * @file
 * VuoMouseButton implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include "type.h"
#include "VuoMouseButton.h"
#include "VuoList_VuoMouseButton.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Mouse Button",
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoMouseButton"
					  ]
				 });
#endif
/// @}

/**
 * @ingroup VuoMouseButton
 * Decodes the JSON object @a js, expected to contain a string, to create a new VuoMouseButton.
 */
VuoMouseButton VuoMouseButton_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoMouseButton value = VuoMouseButton_Left;

	if (! strcmp(valueAsString, "left")) {
		value = VuoMouseButton_Left;
	} else if (! strcmp(valueAsString, "middle")) {
		value = VuoMouseButton_Middle;
	} else if (! strcmp(valueAsString, "right")) {
		value = VuoMouseButton_Right;
	} else if (! strcmp(valueAsString, "any")) {
		value = VuoMouseButton_Any;
	}

	return value;
}

/**
 * @ingroup VuoMouseButton
 * Encodes @a value as a JSON object.
 */
json_object * VuoMouseButton_getJson(const VuoMouseButton value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoMouseButton_Left:
			valueAsString = "left";
			break;
		case VuoMouseButton_Middle:
			valueAsString = "middle";
			break;
		case VuoMouseButton_Right:
			valueAsString = "right";
			break;
		case VuoMouseButton_Any:
			valueAsString = "any";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoMouseButton VuoMouseButton_getAllowedValues(void)
{
	VuoList_VuoMouseButton l = VuoListCreate_VuoMouseButton();
	VuoListAppendValue_VuoMouseButton(l, VuoMouseButton_Left);
	VuoListAppendValue_VuoMouseButton(l, VuoMouseButton_Middle);
	VuoListAppendValue_VuoMouseButton(l, VuoMouseButton_Right);
	VuoListAppendValue_VuoMouseButton(l, VuoMouseButton_Any);
	return l;
}

/**
 * @ingroup VuoMouseButton
 * Same as %VuoMouseButton_getString()
 */
char * VuoMouseButton_getSummary(const VuoMouseButton value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoMouseButton_Left:
			valueAsString = "Left";
			break;
		case VuoMouseButton_Middle:
			valueAsString = "Middle";
			break;
		case VuoMouseButton_Right:
			valueAsString = "Right";
			break;
		case VuoMouseButton_Any:
			valueAsString = "Any";
			break;
	}

	return strdup(valueAsString);
}
