/**
 * @file
 * VuoColorSample C type definition.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoColorSample;
#define VuoList_VuoColorSample_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoColorSample VuoColorSample
 * How to sample a color.
 *
 * @{
 */

/**
 * How to sample a color.
 */
typedef enum
{
	VuoColorSample_Average,
	VuoColorSample_DarkestComponents,
	VuoColorSample_DarkestColor,
	VuoColorSample_LightestComponents,
	VuoColorSample_LightestColor,
} VuoColorSample;

VuoColorSample VuoColorSample_makeFromJson(struct json_object *js);
struct json_object *VuoColorSample_getJson(const VuoColorSample value);
VuoList_VuoColorSample VuoColorSample_getAllowedValues(void);
char *VuoColorSample_getSummary(const VuoColorSample value);

#define VuoColorSample_SUPPORTS_COMPARISON
bool VuoColorSample_areEqual(const VuoColorSample valueA, const VuoColorSample valueB);
bool VuoColorSample_isLessThan(const VuoColorSample valueA, const VuoColorSample valueB);

/**
 * Automatically generated function.
 */
///@{
VuoColorSample VuoColorSample_makeFromString(const char *str);
char *VuoColorSample_getString(const VuoColorSample value);
void VuoColorSample_retain(VuoColorSample value);
void VuoColorSample_release(VuoColorSample value);
///@}

/**
 * @}
 */
