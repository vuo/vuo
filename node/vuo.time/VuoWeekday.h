/**
 * @file
 * VuoWeekday C type definition.
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
 * @defgroup VuoWeekday VuoWeekday
 * A day of the week
 *
 * @{
 */

/**
 * A day of the week
 */
typedef enum
{
	VuoWeekday_Sunday,
	VuoWeekday_Monday,
	VuoWeekday_Tuesday,
	VuoWeekday_Wednesday,
	VuoWeekday_Thursday,
	VuoWeekday_Friday,
	VuoWeekday_Saturday
} VuoWeekday;

#define VuoWeekday_SUPPORTS_COMPARISON
#include "VuoList_VuoWeekday.h"

VuoWeekday VuoWeekday_makeFromJson(struct json_object *js);
struct json_object *VuoWeekday_getJson(const VuoWeekday value);
VuoList_VuoWeekday VuoWeekday_getAllowedValues(void);
char *VuoWeekday_getSummary(const VuoWeekday value);

bool VuoWeekday_areEqual(const VuoWeekday valueA, const VuoWeekday valueB);
bool VuoWeekday_isLessThan(const VuoWeekday valueA, const VuoWeekday valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoWeekday_getString(const VuoWeekday value);
void VuoWeekday_retain(VuoWeekday value);
void VuoWeekday_release(VuoWeekday value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
