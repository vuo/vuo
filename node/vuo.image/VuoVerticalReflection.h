/**
 * @file
 * VuoVerticalReflection C type definition.
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
 * @defgroup VuoVerticalReflection VuoVerticalReflection
 * Options for mirroring an image along the y-axis.
 *
 * @{
 */

/**
 * An enum defining different types of color masks.
 */
typedef enum {
	VuoVerticalReflection_None,
	VuoVerticalReflection_Top,
	VuoVerticalReflection_Bottom
} VuoVerticalReflection;

#include "VuoList_VuoVerticalReflection.h"

VuoVerticalReflection VuoVerticalReflection_makeFromJson(struct json_object * js);
struct json_object * VuoVerticalReflection_getJson(const VuoVerticalReflection value);
VuoList_VuoVerticalReflection VuoVerticalReflection_getAllowedValues(void);
char * VuoVerticalReflection_getSummary(const VuoVerticalReflection value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoVerticalReflection_getString(const VuoVerticalReflection value);
void VuoVerticalReflection_retain(VuoVerticalReflection value);
void VuoVerticalReflection_release(VuoVerticalReflection value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
