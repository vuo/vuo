/**
 * @file
 * VuoAudioFrame C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoAudioFrame VuoAudioFrame
 * A list of VuoAudioSamples (per-channel) and timestamp for one frame of audio.
 * of video.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoAudioSamples.h"
#include "VuoList_VuoAudioSamples.h"
#include <float.h>

/// VuoAudioFrame.timestamp's value when no timestamp is available.
#define VuoAudioFrame_NoTimestamp -INFINITY

/**
* A list of VuoAudioSamples (per-channel) and timestamp for one frame of audio.
 */
typedef struct
{
	VuoList_VuoAudioSamples channels;
	VuoReal timestamp;
} VuoAudioFrame;

#define VuoAudioFrame_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.

VuoAudioFrame VuoAudioFrame_makeFromJson(struct json_object * js);
struct json_object * VuoAudioFrame_getJson(const VuoAudioFrame value);
char * VuoAudioFrame_getSummary(const VuoAudioFrame value);

bool VuoAudioFrame_areEqual(VuoAudioFrame value1, VuoAudioFrame value2);
bool VuoAudioFrame_isLessThan(const VuoAudioFrame a, const VuoAudioFrame b);

void VuoAudioFrame_retain(VuoAudioFrame value);
void VuoAudioFrame_release(VuoAudioFrame value);

/**
 * Automatically generated function.
 */
///@{
char * VuoAudioFrame_getString(const VuoAudioFrame value);
///@}

/**
 * Returns a new VuoAudioFrame.
 */
static inline VuoAudioFrame VuoAudioFrame_make(VuoList_VuoAudioSamples channels, VuoReal timestamp) __attribute__((const));
static inline VuoAudioFrame VuoAudioFrame_make(VuoList_VuoAudioSamples channels, VuoReal timestamp)
{
	return (VuoAudioFrame){channels, timestamp};
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
