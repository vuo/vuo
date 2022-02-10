/**
 * @file
 * VuoNoise C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoNoise_struct { void *l; } * VuoList_VuoNoise;
#define VuoList_VuoNoise_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoNoise VuoNoise
 * An enum defining different types of noise.
 *
 * @{
 */

/**
 * An enum defining different types of noise.
 */
typedef enum {
	VuoNoise_White,
	VuoNoise_Grey,
	VuoNoise_Pink,
	VuoNoise_Brown,
	VuoNoise_Blue,
	VuoNoise_Violet
} VuoNoise;

VuoNoise VuoNoise_makeFromJson(struct json_object * js);
struct json_object * VuoNoise_getJson(const VuoNoise value);
VuoList_VuoNoise VuoNoise_getAllowedValues(void);
char * VuoNoise_getSummary(const VuoNoise value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoNoise_getString(const VuoNoise value);
/// @}

/**
 * @}
*/
