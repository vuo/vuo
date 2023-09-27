/**
 * @file
 * VuoAudioInputDevice C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoAudioInputDevice VuoAudioInputDevice
 * Information about an audio input device.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * Information about an audio input device.
 */
typedef struct
{
	VuoInteger id;	///< If @c id is non-negative, use the specified device identifier.
	VuoText modelUid;	///< If `id` is negative, use the first device whose modelUid contains `modelUid`.
	VuoText name;		///< If `id` is negative and `modelUid` is empty, use the first device whose name matches `name`.  This is a localized display name, so its value will differ depending on the currently-selected system language.

	VuoInteger channelCount;  ///< The number of input channels on this device.  <= 0 if unknown.
} VuoAudioInputDevice;

#define VuoAudioInputDevice_SUPPORTS_COMPARISON

VuoAudioInputDevice VuoAudioInputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoAudioInputDevice_getJson(const VuoAudioInputDevice value);
char * VuoAudioInputDevice_getSummary(const VuoAudioInputDevice value);
char * VuoAudioInputDevice_getShortSummary(const VuoAudioInputDevice value);

bool VuoAudioInputDevice_areEqual(VuoAudioInputDevice value1, VuoAudioInputDevice value2);
bool VuoAudioInputDevice_isLessThan(const VuoAudioInputDevice a, const VuoAudioInputDevice b);

/**
 * Automatically generated function.
 */
///@{
char * VuoAudioInputDevice_getString(const VuoAudioInputDevice value);
void VuoAudioInputDevice_retain(VuoAudioInputDevice value);
void VuoAudioInputDevice_release(VuoAudioInputDevice value);
///@}

/**
 * Returns an audio input device with the specified values.
 */
static inline VuoAudioInputDevice VuoAudioInputDevice_make(VuoInteger id, VuoText modelUid, VuoText name, VuoInteger channelCount) __attribute__((const));
static inline VuoAudioInputDevice VuoAudioInputDevice_make(VuoInteger id, VuoText modelUid, VuoText name, VuoInteger channelCount)
{
	return (VuoAudioInputDevice){id, modelUid, name, channelCount};
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
