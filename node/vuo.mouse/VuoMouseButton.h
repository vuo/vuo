/**
 * @file
 * VuoMouseButton implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMOUSEBUTTON_H
#define VUOMOUSEBUTTON_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoMouseButton VuoMouseButton
 * One or a combination of mouse buttons.
 *
 * @{
 */

/**
 * One or a combination of mouse buttons.
 */
typedef enum {
	VuoMouseButton_Left,
	VuoMouseButton_Middle,
	VuoMouseButton_Right,
	VuoMouseButton_Any
} VuoMouseButton;

VuoMouseButton VuoMouseButton_valueFromJson(struct json_object * js);
struct json_object * VuoMouseButton_jsonFromValue(const VuoMouseButton value);
char * VuoMouseButton_summaryFromValue(const VuoMouseButton value);

/// @{
/**
 * Automatically generated function.
 */
VuoMouseButton VuoMouseButton_valueFromString(const char *str);
char * VuoMouseButton_stringFromValue(const VuoMouseButton value);
/// @}

/**
 * @}
 */

#endif
