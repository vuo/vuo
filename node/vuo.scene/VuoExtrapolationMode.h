/**
 * @file
 * VuoExtrapolationMode C type definition.
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
 * @defgroup VuoExtrapolationMode VuoExtrapolationMode
 * How to extrapolate a list.
 *
 * @{
 */

/**
 * How to extrapolate a list.
 */
typedef enum
{
	VuoExtrapolationMode_Wrap,
	VuoExtrapolationMode_Stretch,
} VuoExtrapolationMode;

#define VuoExtrapolationMode_SUPPORTS_COMPARISON
#include "VuoList_VuoExtrapolationMode.h"

VuoExtrapolationMode VuoExtrapolationMode_makeFromJson(struct json_object *js);
struct json_object *VuoExtrapolationMode_getJson(const VuoExtrapolationMode value);
VuoList_VuoExtrapolationMode VuoExtrapolationMode_getAllowedValues(void);
char *VuoExtrapolationMode_getSummary(const VuoExtrapolationMode value);

bool VuoExtrapolationMode_areEqual(const VuoExtrapolationMode valueA, const VuoExtrapolationMode valueB);
bool VuoExtrapolationMode_isLessThan(const VuoExtrapolationMode valueA, const VuoExtrapolationMode valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoExtrapolationMode_getString(const VuoExtrapolationMode value);
void VuoExtrapolationMode_retain(VuoExtrapolationMode value);
void VuoExtrapolationMode_release(VuoExtrapolationMode value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
