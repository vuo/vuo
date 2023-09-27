/**
 * @file
 * VuoTimeUnit C type definition.
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

/**
 * @ingroup VuoTypes
 * @defgroup VuoTimeUnit VuoTimeUnit
 * A delineation of time.
 *
 * @{
 */

/**
 * A delineation of time.
 */
typedef enum
{
	VuoTimeUnit_Millennium,
	VuoTimeUnit_Century,
	VuoTimeUnit_Decade,
	VuoTimeUnit_Year,
	VuoTimeUnit_Quarter,
	VuoTimeUnit_Month,
	VuoTimeUnit_WeekSunday,
	VuoTimeUnit_WeekMonday,
	VuoTimeUnit_Day,
	VuoTimeUnit_Hour,
	VuoTimeUnit_HalfHour,
	VuoTimeUnit_QuarterHour,
	VuoTimeUnit_Minute,
	VuoTimeUnit_Second,
} VuoTimeUnit;

#define VuoTimeUnit_SUPPORTS_COMPARISON
#include "VuoList_VuoTimeUnit.h"

VuoTimeUnit VuoTimeUnit_makeFromJson(struct json_object *js);
struct json_object *VuoTimeUnit_getJson(const VuoTimeUnit value);
VuoList_VuoTimeUnit VuoTimeUnit_getAllowedValues(void);
char *VuoTimeUnit_getSummary(const VuoTimeUnit value);
VuoInteger VuoTimeUnit_getSeconds(const VuoTimeUnit value);

bool VuoTimeUnit_areEqual(const VuoTimeUnit valueA, const VuoTimeUnit valueB);
bool VuoTimeUnit_isLessThan(const VuoTimeUnit valueA, const VuoTimeUnit valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoTimeUnit_getString(const VuoTimeUnit value);
void VuoTimeUnit_retain(VuoTimeUnit value);
void VuoTimeUnit_release(VuoTimeUnit value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
