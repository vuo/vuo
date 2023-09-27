/**
 * @file
 * VuoHorizontalReflection C type definition.
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
 * @defgroup VuoHorizontalReflection VuoHorizontalReflection
 * Options for mirroring an image along the y-axis.
 *
 * @{
 */

/**
 * An enum defining different types of color masks.
 */
typedef enum {
	VuoHorizontalReflection_None,
	VuoHorizontalReflection_Left,
	VuoHorizontalReflection_Right
} VuoHorizontalReflection;

#include "VuoList_VuoHorizontalReflection.h"

VuoHorizontalReflection VuoHorizontalReflection_makeFromJson(struct json_object * js);
struct json_object * VuoHorizontalReflection_getJson(const VuoHorizontalReflection value);
VuoList_VuoHorizontalReflection VuoHorizontalReflection_getAllowedValues(void);
char * VuoHorizontalReflection_getSummary(const VuoHorizontalReflection value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoHorizontalReflection_getString(const VuoHorizontalReflection value);
void VuoHorizontalReflection_retain(VuoHorizontalReflection value);
void VuoHorizontalReflection_release(VuoHorizontalReflection value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
