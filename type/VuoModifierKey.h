/**
 * @file
 * VuoModifierKey C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMODIFIERKEY_H
#define VUOMODIFIERKEY_H

/// @{
typedef const struct VuoList_VuoModifierKey_struct { void *l; } * VuoList_VuoModifierKey;
#define VuoList_VuoModifierKey_TYPE_DEFINED
/// @}

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

VuoModifierKey VuoModifierKey_makeFromJson(struct json_object * js);
struct json_object * VuoModifierKey_getJson(const VuoModifierKey value);
VuoList_VuoModifierKey VuoModifierKey_getAllowedValues(void);
char * VuoModifierKey_getSummary(const VuoModifierKey value);

bool VuoModifierKey_doMacEventFlagsMatch(unsigned long flags, VuoModifierKey modifierKey);

/// @{
/**
 * Automatically generated function.
 */
VuoModifierKey VuoModifierKey_makeFromString(const char *str);
char * VuoModifierKey_getString(const VuoModifierKey value);
void VuoModifierKey_retain(VuoModifierKey value);
void VuoModifierKey_release(VuoModifierKey value);
/// @}

/**
 * @}
*/

#endif
