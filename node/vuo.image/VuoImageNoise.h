/**
 * @file
 * VuoImageNoise C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoImageNoise;
#define VuoList_VuoImageNoise_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoImageNoise VuoImageNoise
 * An image noise pattern.
 *
 * @{
 */

/**
 * An image noise pattern.
 */
typedef enum
{
	VuoImageNoise_Gradient,
	VuoImageNoise_Value,
	VuoImageNoise_Cellular,
	VuoImageNoise_Dot,
} VuoImageNoise;

VuoImageNoise VuoImageNoise_makeFromJson(struct json_object *js);
struct json_object *VuoImageNoise_getJson(const VuoImageNoise value);
VuoList_VuoImageNoise VuoImageNoise_getAllowedValues(void);
char *VuoImageNoise_getSummary(const VuoImageNoise value);

#define VuoImageNoise_SUPPORTS_COMPARISON
bool VuoImageNoise_areEqual(const VuoImageNoise valueA, const VuoImageNoise valueB);
bool VuoImageNoise_isLessThan(const VuoImageNoise valueA, const VuoImageNoise valueB);

/**
 * Automatically generated function.
 */
///@{
VuoImageNoise VuoImageNoise_makeFromString(const char *str);
char *VuoImageNoise_getString(const VuoImageNoise value);
void VuoImageNoise_retain(VuoImageNoise value);
void VuoImageNoise_release(VuoImageNoise value);
///@}

/**
 * @}
 */
