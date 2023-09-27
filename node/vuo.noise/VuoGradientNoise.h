/**
 * @file
 * VuoGradientNoise C type definition.
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
 * @defgroup VuoGradientNoise VuoGradientNoise
 * An enum defining different types of noise.
 *
 * @{
 */

/**
 * An enum defining different types of noise.
 */
typedef enum {
	VuoGradientNoise_Rectangular,
	VuoGradientNoise_Triangular
} VuoGradientNoise;

#include "VuoList_VuoGradientNoise.h"

VuoGradientNoise VuoGradientNoise_makeFromJson(struct json_object * js);
struct json_object * VuoGradientNoise_getJson(const VuoGradientNoise value);
VuoList_VuoGradientNoise VuoGradientNoise_getAllowedValues(void);
char * VuoGradientNoise_getSummary(const VuoGradientNoise value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoGradientNoise_getString(const VuoGradientNoise value);
void VuoGradientNoise_retain(VuoGradientNoise value);
void VuoGradientNoise_release(VuoGradientNoise value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
