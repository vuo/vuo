/**
 * @file
 * VuoModifierKey C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMODIFIERKEY_H
#define VUOMODIFIERKEY_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoModifierKey VuoModifierKey
 * A modifier key from the keyboard that may accompany a mouse action.
 *
 * @{
 */

/**
 * The available modifier keys (or any or none).
 */
typedef enum {
	VuoModifierKey_Any,
	VuoModifierKey_Command,
	VuoModifierKey_Option,
	VuoModifierKey_Control,
	VuoModifierKey_Shift,
	VuoModifierKey_None
} VuoModifierKey;

VuoModifierKey VuoModifierKey_valueFromJson(struct json_object * js);
struct json_object * VuoModifierKey_jsonFromValue(const VuoModifierKey value);
char * VuoModifierKey_summaryFromValue(const VuoModifierKey value);

bool VuoModifierKey_doMacEventFlagsMatch(unsigned long flags, VuoModifierKey modifierKey);

/// @{
/**
 * Automatically generated function.
 */
VuoModifierKey VuoModifierKey_valueFromString(const char *str);
char * VuoModifierKey_stringFromValue(const VuoModifierKey value);
/// @}

/**
 * @}
*/

#endif
