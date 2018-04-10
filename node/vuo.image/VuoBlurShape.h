/**
 * @file
 * VuoBlurShape C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoBlurShape;
#define VuoList_VuoBlurShape_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoBlurShape VuoBlurShape
 * Weights for pixels in a blur
 *
 * @{
 */

/**
 * Weights for pixels in a blur
 */
typedef enum
{
	VuoBlurShape_Gaussian,
	VuoBlurShape_Linear,
	VuoBlurShape_Box,
	VuoBlurShape_Disc,
} VuoBlurShape;

VuoBlurShape VuoBlurShape_makeFromJson(struct json_object *js);
struct json_object *VuoBlurShape_getJson(const VuoBlurShape value);
VuoList_VuoBlurShape VuoBlurShape_getAllowedValues(void);
char *VuoBlurShape_getSummary(const VuoBlurShape value);

#define VuoBlurShape_SUPPORTS_COMPARISON
bool VuoBlurShape_areEqual(const VuoBlurShape valueA, const VuoBlurShape valueB);
bool VuoBlurShape_isLessThan(const VuoBlurShape valueA, const VuoBlurShape valueB);

/**
 * Automatically generated function.
 */
///@{
VuoBlurShape VuoBlurShape_makeFromString(const char *str);
char *VuoBlurShape_getString(const VuoBlurShape value);
void VuoBlurShape_retain(VuoBlurShape value);
void VuoBlurShape_release(VuoBlurShape value);
///@}

/**
 * @}
 */
