/**
 * @file
 * VuoTime C type definition.
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
#include "VuoWeekday.h"
#include "VuoText.h"
#include "VuoTimeFormat.h"
#include "VuoTimeUnit.h"

#include <xlocale.h>

/**
 * @ingroup VuoTypes
 * @defgroup VuoTime VuoTime
 * A date and time.
 *
 * @{
 */

/**
 * A date and time.
 *
 * The number of seconds since 2001.01.01 @ 00:00:00 UTC, including fractional seconds.
 *
 * Regarding precision: A 64-bit double can represent 15 significant figures in decimal.
 * In 2015 (about 441,000,000 seconds since 2001.01.01),
 * that leaves 6 significant figures to represent fractional seconds,
 * meaning VuoTime can represent individual microseconds.
 *
 * This type is similar in definition to Cocoa's `NSDate` and Core Foundation's `CFDateRef` —
 * it's timezone-independent / always in UTC.
 */
typedef double VuoTime;

#define VuoTime_SUPPORTS_COMPARISON
#include "VuoList_VuoTime.h"

VuoTime VuoTime_makeFromJson(struct json_object *js);
struct json_object *VuoTime_getJson(const VuoTime value);
char *VuoTime_getSummary(const VuoTime value);

bool VuoTime_areEqual(const VuoTime valueA, const VuoTime valueB);
bool VuoTime_isLessThan(const VuoTime valueA, const VuoTime valueB);

bool VuoTime_areEqualWithinTolerance(VuoList_VuoTime times, VuoReal tolerance, VuoTimeUnit toleranceUnit);
bool VuoTime_areTimesOfDayEqualWithinTolerance(VuoList_VuoTime times, VuoReal tolerance, VuoTimeUnit toleranceUnit);
bool VuoTime_isTimeOfDayLessThan(const VuoTime valueA, const VuoTime valueB, const VuoTime startOfDay);

VuoTime VuoTime_getCurrent(void);
VuoTime VuoTime_make(VuoInteger year, VuoInteger month, VuoInteger dayOfMonth, VuoInteger hour, VuoInteger minute, VuoReal second);
VuoTime VuoTime_makeFromRFC822(const char *rfc822);
VuoTime VuoTime_makeFromISO8601(const char *iso8601);
VuoTime VuoTime_makeFromUnknownFormat(const char *str);
bool VuoTime_getComponents(VuoTime time, VuoInteger *year, VuoInteger *dayOfYear, VuoInteger *month, VuoInteger *dayOfMonth, VuoInteger *week, VuoWeekday *dayOfWeek, VuoInteger *hour, VuoInteger *minute, VuoReal *second) VuoWarnUnusedResult;
VuoTime VuoTime_round(const VuoTime value, const VuoTimeUnit unit, const int roundingMethod);
VuoText VuoTime_format(const VuoTime time, const VuoTimeFormat format);
VuoText VuoTime_formatWithLocale(const VuoTime time, const VuoTimeFormat format, locale_t locale);

/**
 * Automatically generated function.
 */
///@{
char *VuoTime_getString(const VuoTime value);
void VuoTime_retain(VuoTime value);
void VuoTime_release(VuoTime value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
