/**
 * @file
 * VuoParity C type definition.
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
 * @defgroup VuoParity VuoParity
 * Whether to include an error detection bit, and how to use it.
 *
 * @{
 */

/**
 * Whether to include an error detection bit, and how to use it.
 */
typedef enum
{
	VuoParity_None,
	VuoParity_Even,
	VuoParity_Odd
} VuoParity;

#define VuoParity_SUPPORTS_COMPARISON
#include "VuoList_VuoParity.h"

VuoParity VuoParity_makeFromJson(struct json_object *js);
struct json_object *VuoParity_getJson(const VuoParity value);
VuoList_VuoParity VuoParity_getAllowedValues(void);
char *VuoParity_getSummary(const VuoParity value);

bool VuoParity_areEqual(const VuoParity valueA, const VuoParity valueB);
bool VuoParity_isLessThan(const VuoParity valueA, const VuoParity valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoParity_getString(const VuoParity value);
void VuoParity_retain(VuoParity value);
void VuoParity_release(VuoParity value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
