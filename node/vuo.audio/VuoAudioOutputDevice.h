/**
 * @file
 * VuoAudioOutputDevice C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoAudioOutputDevice VuoAudioOutputDevice
 * Information about an audio output device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * Information about an audio ouput device.
 */
typedef struct
{
	VuoInteger id;	///< If @c id is non-negative, use the specified device identifier.
	VuoText modelUid;	///< If `id` is negative, use the first device whose modelUid contains `modelUid`.
	VuoText name;		///< If `id` is negative and `modelUid` is empty, use the first device whose name matches `name`.  This is a localized display name, so its value will differ depending on the currently-selected system language.

	VuoInteger channelCount;	///< The number of output channels on this device.
} VuoAudioOutputDevice;

VuoAudioOutputDevice VuoAudioOutputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoAudioOutputDevice_getJson(const VuoAudioOutputDevice value);
char * VuoAudioOutputDevice_getSummary(const VuoAudioOutputDevice value);
bool VuoAudioOutputDevice_areEqual(VuoAudioOutputDevice value1, VuoAudioOutputDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoAudioOutputDevice VuoAudioOutputDevice_makeFromString(const char *str);
char * VuoAudioOutputDevice_getString(const VuoAudioOutputDevice value);
void VuoAudioOutputDevice_retain(VuoAudioOutputDevice value);
void VuoAudioOutputDevice_release(VuoAudioOutputDevice value);
///@}

/**
 * Returns an audio output device with the specified values.
 */
static inline VuoAudioOutputDevice VuoAudioOutputDevice_make(VuoInteger id, VuoText modelUid, VuoText name, VuoInteger channelCount) __attribute__((const));
static inline VuoAudioOutputDevice VuoAudioOutputDevice_make(VuoInteger id, VuoText modelUid, VuoText name, VuoInteger channelCount)
{
	return (VuoAudioOutputDevice){id, modelUid, name, channelCount};
}

/**
 * @}
 */

