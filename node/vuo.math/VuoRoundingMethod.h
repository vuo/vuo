/**
 * @file
 * VuoRoundingMethod C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoRoundingMethod;
#define VuoList_VuoRoundingMethod_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoRoundingMethod VuoRoundingMethod
 * How to round a number.
 *
 * @{
 */

/**
 * How to round a number.
 */
typedef enum
{
	VuoRoundingMethod_Nearest,
	VuoRoundingMethod_Down,
	VuoRoundingMethod_Up
} VuoRoundingMethod;

VuoRoundingMethod VuoRoundingMethod_makeFromJson(struct json_object *js);
struct json_object *VuoRoundingMethod_getJson(const VuoRoundingMethod value);
VuoList_VuoRoundingMethod VuoRoundingMethod_getAllowedValues(void);
char *VuoRoundingMethod_getSummary(const VuoRoundingMethod value);

#define VuoRoundingMethod_SUPPORTS_COMPARISON
bool VuoRoundingMethod_areEqual(const VuoRoundingMethod valueA, const VuoRoundingMethod valueB);
bool VuoRoundingMethod_isLessThan(const VuoRoundingMethod valueA, const VuoRoundingMethod valueB);

/**
 * Automatically generated function.
 */
///@{
VuoRoundingMethod VuoRoundingMethod_makeFromString(const char *str);
char *VuoRoundingMethod_getString(const VuoRoundingMethod value);
void VuoRoundingMethod_retain(VuoRoundingMethod value);
void VuoRoundingMethod_release(VuoRoundingMethod value);
///@}

/**
 * @}
 */
