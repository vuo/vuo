/**
 * @file
 * VuoRelativeTime C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include "VuoReal.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoRelativeTime VuoRelativeTime
 * An offset from a Date-Time.
 *
 * @{
 */

/**
 * An offset from a Date-Time.
 *
 * The number of seconds since another time, including fractional seconds.
 */
typedef double VuoRelativeTime;

#define VuoRelativeTime_SUPPORTS_COMPARISON

VuoRelativeTime VuoRelativeTime_makeFromJson(struct json_object *js);
struct json_object *VuoRelativeTime_getJson(const VuoRelativeTime value);
char *VuoRelativeTime_getSummary(const VuoRelativeTime value);

bool VuoRelativeTime_areEqual(const VuoRelativeTime valueA, const VuoRelativeTime valueB);
bool VuoRelativeTime_isLessThan(const VuoRelativeTime valueA, const VuoRelativeTime valueB);

VuoRelativeTime VuoRelativeTime_make(VuoInteger years, VuoInteger months, VuoInteger days, VuoInteger hours, VuoInteger minutes, VuoReal seconds);
void VuoRelativeTime_getComponents(VuoRelativeTime relativeTime, VuoInteger *years, VuoInteger *months, VuoInteger *days, VuoInteger *hours, VuoInteger *minutes, VuoReal *seconds);

/**
 * Automatically generated function.
 */
///@{
char *VuoRelativeTime_getString(const VuoRelativeTime value);
void VuoRelativeTime_retain(VuoRelativeTime value);
void VuoRelativeTime_release(VuoRelativeTime value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
