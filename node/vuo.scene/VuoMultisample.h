/**
 * @file
 * VuoMultisample C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoMultisample;
#define VuoList_VuoMultisample_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoMultisample VuoMultisample
 * Number of samples per pixel.
 *
 * @{
 */

/**
 * Number of samples per pixel.
 */
typedef enum
{
	VuoMultisample_Off = 0,
	VuoMultisample_2 = 2,
	VuoMultisample_4 = 4,
	VuoMultisample_8 = 8,
} VuoMultisample;

VuoMultisample VuoMultisample_makeFromJson(struct json_object *js);
struct json_object *VuoMultisample_getJson(const VuoMultisample value);
VuoList_VuoMultisample VuoMultisample_getAllowedValues(void);
char *VuoMultisample_getSummary(const VuoMultisample value);

#define VuoMultisample_SUPPORTS_COMPARISON
bool VuoMultisample_areEqual(const VuoMultisample valueA, const VuoMultisample valueB);
bool VuoMultisample_isLessThan(const VuoMultisample valueA, const VuoMultisample valueB);

/**
 * Automatically generated function.
 */
///@{
VuoMultisample VuoMultisample_makeFromString(const char *str);
char *VuoMultisample_getString(const VuoMultisample value);
void VuoMultisample_retain(VuoMultisample value);
void VuoMultisample_release(VuoMultisample value);
///@}

/**
 * @}
 */

