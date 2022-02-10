/**
 * @file
 * VuoMidiOutputDevice C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoMidiOutputDevice VuoMidiOutputDevice
 * A set of specifications for choosing a MIDI output device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * A set of specifications for choosing a MIDI output device.
 */
typedef struct
{
	VuoInteger id;	///< If @c id is non-negative, use the specified device identifier.
	VuoText name;	///< If @c id is negative, use the first device whose name contains @c name.
} VuoMidiOutputDevice;

VuoMidiOutputDevice VuoMidiOutputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoMidiOutputDevice_getJson(const VuoMidiOutputDevice value);
char * VuoMidiOutputDevice_getSummary(const VuoMidiOutputDevice value);

#define VuoMidiOutputDevice_SUPPORTS_COMPARISON
bool VuoMidiOutputDevice_areEqual(const VuoMidiOutputDevice value1, const VuoMidiOutputDevice value2);
bool VuoMidiOutputDevice_isLessThan(const VuoMidiOutputDevice a, const VuoMidiOutputDevice b);

void VuoMidiOutputDevice_retain(VuoMidiOutputDevice value);
void VuoMidiOutputDevice_release(VuoMidiOutputDevice value);

/**
 * Automatically generated function.
 */
///@{
char * VuoMidiOutputDevice_getString(const VuoMidiOutputDevice value);
///@}

/**
 * Returns a MIDI device with the specified values.
 */
static inline VuoMidiOutputDevice VuoMidiOutputDevice_make(VuoInteger id, VuoText name) __attribute__((const));
static inline VuoMidiOutputDevice VuoMidiOutputDevice_make(VuoInteger id, VuoText name)
{
	return (VuoMidiOutputDevice){id, name};
}

/**
 * @}
 */
