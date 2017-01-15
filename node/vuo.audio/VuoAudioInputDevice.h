/**
 * @file
 * VuoAudioInputDevice C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOAUDIOINPUTDEVICE_H
#define VUOAUDIOINPUTDEVICE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoAudioInputDevice VuoAudioInputDevice
 * Information about an audio input device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * Information about an audio input device.
 */
typedef struct
{
	VuoInteger id;	///< If @c id is non-negative, use the specified device identifier.
	VuoText name;	///< If @c id is negative, use the first device whose name contains @c name.

	VuoInteger channelCount;	///< The number of input channels on this device.
} VuoAudioInputDevice;

VuoAudioInputDevice VuoAudioInputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoAudioInputDevice_getJson(const VuoAudioInputDevice value);
char * VuoAudioInputDevice_getSummary(const VuoAudioInputDevice value);
bool VuoAudioInputDevice_areEqual(VuoAudioInputDevice value1, VuoAudioInputDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoAudioInputDevice VuoAudioInputDevice_makeFromString(const char *str);
char * VuoAudioInputDevice_getString(const VuoAudioInputDevice value);
void VuoAudioInputDevice_retain(VuoAudioInputDevice value);
void VuoAudioInputDevice_release(VuoAudioInputDevice value);
///@}

/**
 * Returns an audio input device with the specified values.
 */
static inline VuoAudioInputDevice VuoAudioInputDevice_make(VuoInteger id, VuoText name, VuoInteger channelCount) __attribute__((const));
static inline VuoAudioInputDevice VuoAudioInputDevice_make(VuoInteger id, VuoText name, VuoInteger channelCount)
{
	VuoAudioInputDevice aid = {id,name,channelCount};
	return aid;
}

/**
 * @}
 */

#endif // VUOAUDIOINPUTDEVICE_H

