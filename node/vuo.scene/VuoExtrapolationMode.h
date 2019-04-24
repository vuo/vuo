/**
 * @file
 * VuoExtrapolationMode C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoExtrapolationMode;
#define VuoList_VuoExtrapolationMode_TYPE_DEFINED
/// @}

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

VuoExtrapolationMode VuoExtrapolationMode_makeFromJson(struct json_object *js);
struct json_object *VuoExtrapolationMode_getJson(const VuoExtrapolationMode value);
VuoList_VuoExtrapolationMode VuoExtrapolationMode_getAllowedValues(void);
char *VuoExtrapolationMode_getSummary(const VuoExtrapolationMode value);

#define VuoExtrapolationMode_SUPPORTS_COMPARISON
bool VuoExtrapolationMode_areEqual(const VuoExtrapolationMode valueA, const VuoExtrapolationMode valueB);
bool VuoExtrapolationMode_isLessThan(const VuoExtrapolationMode valueA, const VuoExtrapolationMode valueB);

/**
 * Automatically generated function.
 */
///@{
VuoExtrapolationMode VuoExtrapolationMode_makeFromString(const char *str);
char *VuoExtrapolationMode_getString(const VuoExtrapolationMode value);
void VuoExtrapolationMode_retain(VuoExtrapolationMode value);
void VuoExtrapolationMode_release(VuoExtrapolationMode value);
///@}

/**
 * @}
 */
