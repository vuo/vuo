/**
 * @file
 * VuoMultisample C type definition.
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
 * @defgroup VuoMultisample VuoMultisample
 * Number of samples per pixel.
 *
 * @{
 */

/**
 * Number of samples per pixel.
 */
typedef enum
{
	VuoMultisample_Off = 0,
	VuoMultisample_2 = 2,
	VuoMultisample_4 = 4,
	VuoMultisample_8 = 8,
} VuoMultisample;

#define VuoMultisample_SUPPORTS_COMPARISON
#include "VuoList_VuoMultisample.h"

VuoMultisample VuoMultisample_makeFromJson(struct json_object *js);
struct json_object *VuoMultisample_getJson(const VuoMultisample value);
VuoList_VuoMultisample VuoMultisample_getAllowedValues(void);
char *VuoMultisample_getSummary(const VuoMultisample value);

bool VuoMultisample_areEqual(const VuoMultisample valueA, const VuoMultisample valueB);
bool VuoMultisample_isLessThan(const VuoMultisample valueA, const VuoMultisample valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoMultisample_getString(const VuoMultisample value);
void VuoMultisample_retain(VuoMultisample value);
void VuoMultisample_release(VuoMultisample value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
