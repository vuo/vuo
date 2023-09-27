/**
 * @file
 * VuoImageStereoType C type definition.
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
 * @defgroup VuoImageStereoType VuoImageStereoType
 * Defines the color mask to be applied.
 *
 * @{
 */

/**
 * An enum defining different types of color masks.
 */
typedef enum {
	VuoImageStereoType_LeftRightHalf,
	VuoImageStereoType_LeftRightFull,
	VuoImageStereoType_TopBottomHalf,
	VuoImageStereoType_TopBottomFull,
	VuoImageStereoType_AnaglyphRedCyan,
	VuoImageStereoType_AnaglyphAmberBlue,
	VuoImageStereoType_AnaglyphGreenMagenta,
	VuoImageStereoType_VerticalStripe,
	VuoImageStereoType_HorizontalStripe,
	VuoImageStereoType_Checkerboard,
	VuoImageStereoType_Blend,
	VuoImageStereoType_Difference,
	VuoImageStereoType_LeftOnly,
	VuoImageStereoType_RightOnly
} VuoImageStereoType;

#include "VuoList_VuoImageStereoType.h"

VuoImageStereoType VuoImageStereoType_makeFromJson(struct json_object * js);
struct json_object * VuoImageStereoType_getJson(const VuoImageStereoType value);
VuoList_VuoImageStereoType VuoImageStereoType_getAllowedValues(void);
char * VuoImageStereoType_getSummary(const VuoImageStereoType value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoImageStereoType_getString(const VuoImageStereoType value);
void VuoImageStereoType_retain(VuoImageStereoType value);
void VuoImageStereoType_release(VuoImageStereoType value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
