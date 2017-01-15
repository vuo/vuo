/**
 * @file
 * VuoMouseButton implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMOUSEBUTTON_H
#define VUOMOUSEBUTTON_H

/// @{
typedef const struct VuoList_VuoMouseButton_struct { void *l; } * VuoList_VuoMouseButton;
#define VuoList_VuoMouseButton_TYPE_DEFINED
/// @}

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

VuoMouseButton VuoMouseButton_makeFromJson(struct json_object * js);
struct json_object * VuoMouseButton_getJson(const VuoMouseButton value);
VuoList_VuoMouseButton VuoMouseButton_getAllowedValues(void);
char * VuoMouseButton_getSummary(const VuoMouseButton value);

/// @{
/**
 * Automatically generated function.
 */
VuoMouseButton VuoMouseButton_makeFromString(const char *str);
char * VuoMouseButton_getString(const VuoMouseButton value);
/// @}

/**
 * @}
 */

#endif
