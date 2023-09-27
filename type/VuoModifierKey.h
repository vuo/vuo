/**
 * @file
 * VuoModifierKey C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoModifierKey_h
#define VuoModifierKey_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoModifierKey VuoModifierKey
 * A modifier key from the keyboard that may accompany a mouse action.
 *
 * @{
 */

/**
 * The available modifier keys (or any or none).
 * Values are stored in high 32 bits of a 64 bit unsigned int.
 */
typedef enum {
	VuoModifierKey_Any 		= 0xFFFF,
	VuoModifierKey_Command 	= 0x10,
	VuoModifierKey_Option 	= 0x8,
	VuoModifierKey_Control	= 0x4,
	VuoModifierKey_Shift	= 0x2,
	VuoModifierKey_None		= 0x0
} VuoModifierKey;

#include "VuoList_VuoModifierKey.h"

VuoModifierKey VuoModifierKey_makeFromJson(struct json_object * js);
struct json_object * VuoModifierKey_getJson(const VuoModifierKey value);
VuoList_VuoModifierKey VuoModifierKey_getAllowedValues(void);
char * VuoModifierKey_getSummary(const VuoModifierKey value);

bool VuoModifierKey_doMacEventFlagsMatch(unsigned long flags, VuoModifierKey modifierKey);

/// @{
/**
 * Automatically generated function.
 */
char * VuoModifierKey_getString(const VuoModifierKey value);
void VuoModifierKey_retain(VuoModifierKey value);
void VuoModifierKey_release(VuoModifierKey value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

#endif
