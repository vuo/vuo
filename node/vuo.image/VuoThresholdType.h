/**
 * @file
 * VuoThresholdType C type definition.
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
 * @defgroup VuoThresholdType VuoThresholdType
 * Defines the color mask to be applied.
 *
 * @{
 */

/**
 * An enum defining different types of color masks.
 */
typedef enum {
	VuoThresholdType_Rec601, // ITU-R Recommendation BT.601 (1982) — NTSC / CCIR
	VuoThresholdType_Rec709, // ITU-R Recommendation BT.709 (1990) — HDTV
	VuoThresholdType_Desaturate, // HSL: S=0
	VuoThresholdType_RGBAverage,
	VuoThresholdType_RGBMaximum,
	VuoThresholdType_RGBMinimum,
	VuoThresholdType_Red,
	VuoThresholdType_Green,
	VuoThresholdType_Blue,
	VuoThresholdType_Alpha,
	VuoThresholdType_RGB,
} VuoThresholdType;

#include "VuoList_VuoThresholdType.h"

VuoThresholdType VuoThresholdType_makeFromJson(struct json_object * js);
struct json_object * VuoThresholdType_getJson(const VuoThresholdType value);
VuoList_VuoThresholdType VuoThresholdType_getAllowedValues(void);
char * VuoThresholdType_getSummary(const VuoThresholdType value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoThresholdType_getString(const VuoThresholdType value);
void VuoThresholdType_retain(VuoThresholdType value);
void VuoThresholdType_release(VuoThresholdType value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
