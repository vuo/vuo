/**
 * @file
 * VuoTimeFormat C type definition.
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
 * @defgroup VuoTimeFormat VuoTimeFormat
 * How to format a Date-Time.
 *
 * @{
 */

/**
 * How to format a Date-Time.
 */
typedef enum
{
	VuoTimeFormat_DateTimeSortable,	// ISO 8601 / RFC 3339.  Always in UTC.  Locale-independent.
	VuoTimeFormat_DateTimeShort12,	// Others use the current local timezone and locale.
	VuoTimeFormat_DateTimeShort24,
	VuoTimeFormat_DateTimeMedium12,
	VuoTimeFormat_DateTimeMedium24,
	VuoTimeFormat_DateTimeLong12,
	VuoTimeFormat_DateTimeLong24,
	VuoTimeFormat_DateShort,
	VuoTimeFormat_DateMedium,
	VuoTimeFormat_DateLong,
	VuoTimeFormat_Time12,
	VuoTimeFormat_Time24,
} VuoTimeFormat;

#define VuoTimeFormat_SUPPORTS_COMPARISON
#include "VuoList_VuoTimeFormat.h"

VuoTimeFormat VuoTimeFormat_makeFromJson(struct json_object *js);
struct json_object *VuoTimeFormat_getJson(const VuoTimeFormat value);
VuoList_VuoTimeFormat VuoTimeFormat_getAllowedValues(void);
char *VuoTimeFormat_getSummary(const VuoTimeFormat value);

bool VuoTimeFormat_areEqual(const VuoTimeFormat valueA, const VuoTimeFormat valueB);
bool VuoTimeFormat_isLessThan(const VuoTimeFormat valueA, const VuoTimeFormat valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoTimeFormat_getString(const VuoTimeFormat value);
void VuoTimeFormat_retain(VuoTimeFormat value);
void VuoTimeFormat_release(VuoTimeFormat value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
