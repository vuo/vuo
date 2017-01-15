/**
 * @file
 * VuoModifierKey implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include <Carbon/Carbon.h>
#include "type.h"
#include "VuoModifierKey.h"
#include "VuoList_VuoModifierKey.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Modifier Key",
					  "dependencies" : [
						"VuoList_VuoModifierKey",
						"Carbon.framework"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoModifierKey
 * Decodes the JSON object @a js, expected to contain a string, to create a new VuoModifierKey.
 */
VuoModifierKey VuoModifierKey_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoModifierKey value = VuoModifierKey_Any;

	if (! strcmp(valueAsString, "any")) {
		value = VuoModifierKey_Any;
	} else if (! strcmp(valueAsString, "command")) {
		value = VuoModifierKey_Command;
	} else if (! strcmp(valueAsString, "option")) {
		value = VuoModifierKey_Option;
	} else if (! strcmp(valueAsString, "control")) {
		value = VuoModifierKey_Control;
	} else if (! strcmp(valueAsString, "shift")) {
		value = VuoModifierKey_Shift;
	} else if (! strcmp(valueAsString, "none")) {
		value = VuoModifierKey_None;
	}

	return value;
}

/**
 * @ingroup VuoModifierKey
 * Encodes @a value as a JSON object.
 */
json_object * VuoModifierKey_getJson(const VuoModifierKey value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoModifierKey_Any:
			valueAsString = "any";
			break;
		case VuoModifierKey_Command:
			valueAsString = "command";
			break;
		case VuoModifierKey_Option:
			valueAsString = "option";
			break;
		case VuoModifierKey_Control:
			valueAsString = "control";
			break;
		case VuoModifierKey_Shift:
			valueAsString = "shift";
			break;
		case VuoModifierKey_None:
			valueAsString = "none";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoModifierKey VuoModifierKey_getAllowedValues(void)
{
	VuoList_VuoModifierKey l = VuoListCreate_VuoModifierKey();
	VuoListAppendValue_VuoModifierKey(l, VuoModifierKey_Any);
	VuoListAppendValue_VuoModifierKey(l, VuoModifierKey_Command);
	VuoListAppendValue_VuoModifierKey(l, VuoModifierKey_Option);
	VuoListAppendValue_VuoModifierKey(l, VuoModifierKey_Control);
	VuoListAppendValue_VuoModifierKey(l, VuoModifierKey_Shift);
	VuoListAppendValue_VuoModifierKey(l, VuoModifierKey_None);
	return l;
}

/**
 * @ingroup VuoModifierKey
 * Returns a brief description of @a value.
 */
char * VuoModifierKey_getSummary(const VuoModifierKey value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoModifierKey_Any:
			valueAsString = "Any";
			break;
		case VuoModifierKey_Command:
			valueAsString = "Command";
			break;
		case VuoModifierKey_Option:
			valueAsString = "Option";
			break;
		case VuoModifierKey_Control:
			valueAsString = "Control";
			break;
		case VuoModifierKey_Shift:
			valueAsString = "Shift";
			break;
		case VuoModifierKey_None:
			valueAsString = "None";
			break;
	}

	return strdup(valueAsString);
}

/**
 * Returns true if @a flags (of type @c CGEventFlags) contains the given modifier keys.
 */
bool VuoModifierKey_doMacEventFlagsMatch(unsigned long flags, VuoModifierKey modifierKey)
{
	bool isCommand = (flags & kCGEventFlagMaskCommand);
	bool isOption = (flags & kCGEventFlagMaskAlternate);
	bool isControl = (flags & kCGEventFlagMaskControl);
	bool isShift = (flags & kCGEventFlagMaskShift);

	switch (modifierKey)
	{
		case VuoModifierKey_Any:
			return true;
		case VuoModifierKey_Command:
			return isCommand;
		case VuoModifierKey_Option:
			return isOption;
		case VuoModifierKey_Control:
			return isControl;
		case VuoModifierKey_Shift:
			return isShift;
		case VuoModifierKey_None:
			return ! isCommand && ! isOption && ! isControl && ! isShift;
	}

	return false;
}
