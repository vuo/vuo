/**
 * @file
 * VuoColorSample C type definition.
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

#define VuoColorSample_SUPPORTS_COMPARISON
#include "VuoList_VuoColorSample.h"

VuoColorSample VuoColorSample_makeFromJson(struct json_object *js);
struct json_object *VuoColorSample_getJson(const VuoColorSample value);
VuoList_VuoColorSample VuoColorSample_getAllowedValues(void);
char *VuoColorSample_getSummary(const VuoColorSample value);

bool VuoColorSample_areEqual(const VuoColorSample valueA, const VuoColorSample valueB);
bool VuoColorSample_isLessThan(const VuoColorSample valueA, const VuoColorSample valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoColorSample_getString(const VuoColorSample value);
void VuoColorSample_retain(VuoColorSample value);
void VuoColorSample_release(VuoColorSample value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
