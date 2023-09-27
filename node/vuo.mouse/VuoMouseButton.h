/**
 * @file
 * VuoMouseButton implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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

#include "VuoList_VuoMouseButton.h"

VuoMouseButton VuoMouseButton_makeFromJson(struct json_object * js);
struct json_object * VuoMouseButton_getJson(const VuoMouseButton value);
VuoList_VuoMouseButton VuoMouseButton_getAllowedValues(void);
char * VuoMouseButton_getSummary(const VuoMouseButton value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoMouseButton_getString(const VuoMouseButton value);
void VuoMouseButton_retain(VuoMouseButton value);
void VuoMouseButton_release(VuoMouseButton value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
