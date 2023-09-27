/**
 * @file
 * VuoHorizontalSide C type definition.
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
 * @defgroup VuoHorizontalSide VuoHorizontalSide
 * An enum defining direction on the horizontal axis (right or left).
 *
 * @{
 */

/**
 * Direction on the horizontal axis.
 */
typedef enum {
	VuoHorizontalSide_Left,			//  8 bits per channel (32 bits per RGBA pixel)
	VuoHorizontalSide_Right			// 16 bits per channel (64 bits per RGBA pixel)
} VuoHorizontalSide;

#include "VuoList_VuoHorizontalSide.h"

VuoHorizontalSide VuoHorizontalSide_makeFromJson(struct json_object * js);
struct json_object * VuoHorizontalSide_getJson(const VuoHorizontalSide value);
VuoList_VuoHorizontalSide VuoHorizontalSide_getAllowedValues(void);
char * VuoHorizontalSide_getSummary(const VuoHorizontalSide value);

/**
 * Automatically generated function.
 */
///@{
char * VuoHorizontalSide_getString(const VuoHorizontalSide value);
void VuoHorizontalSide_retain(VuoHorizontalSide value);
void VuoHorizontalSide_release(VuoHorizontalSide value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
