/**
 * @file
 * VuoVideoOptimization C type definition.
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
 * @defgroup VuoVideoOptimization VuoVideoOptimization
 * An enum defining different video decoder quality settings (prefer forward playback or random decode performance).
 *
 * @{
 */

/**
 * An enum defining different video decoder quality settings (prefer forward playback or random decode performance).
 * When set to Auto, VuoVideoPlayer will automatically choose the decoder to use based on the current video player
 * settings.  When set to Forward AvFoundation is preferred over Ffmpeg, and vice-versa for Random.
 */
typedef enum {
	VuoVideoOptimization_Auto,
	VuoVideoOptimization_Forward,
	VuoVideoOptimization_Random
} VuoVideoOptimization;

#include "VuoList_VuoVideoOptimization.h"

VuoVideoOptimization VuoVideoOptimization_makeFromJson(struct json_object * js);
struct json_object * VuoVideoOptimization_getJson(const VuoVideoOptimization value);
VuoList_VuoVideoOptimization VuoVideoOptimization_getAllowedValues(void);
char * VuoVideoOptimization_getSummary(const VuoVideoOptimization value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoVideoOptimization_getString(const VuoVideoOptimization value);
void VuoVideoOptimization_retain(VuoVideoOptimization value);
void VuoVideoOptimization_release(VuoVideoOptimization value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
