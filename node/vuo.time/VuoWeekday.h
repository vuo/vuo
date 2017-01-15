/**
 * @file
 * VuoWeekday C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoWeekday;
#define VuoList_VuoWeekday_TYPE_DEFINED
/// @}

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

VuoWeekday VuoWeekday_makeFromJson(struct json_object *js);
struct json_object *VuoWeekday_getJson(const VuoWeekday value);
VuoList_VuoWeekday VuoWeekday_getAllowedValues(void);
char *VuoWeekday_getSummary(const VuoWeekday value);

#define VuoWeekday_SUPPORTS_COMPARISON
bool VuoWeekday_areEqual(const VuoWeekday valueA, const VuoWeekday valueB);
bool VuoWeekday_isLessThan(const VuoWeekday valueA, const VuoWeekday valueB);

/**
 * Automatically generated function.
 */
///@{
VuoWeekday VuoWeekday_makeFromString(const char *str);
char *VuoWeekday_getString(const VuoWeekday value);
void VuoWeekday_retain(VuoWeekday value);
void VuoWeekday_release(VuoWeekday value);
///@}

/**
 * @}
 */
