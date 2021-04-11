/**
 * @file
 * VuoMidiInputDevice C type definition.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoMidiInputDevice VuoMidiInputDevice
 * A set of specifications for choosing a MIDI input device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * A set of specifications for choosing a MIDI input device.
 */
typedef struct
{
	VuoInteger id;	///< If @c id is non-negative, use the specified device identifier.
	VuoText name;	///< If @c id is negative, use the first device whose name contains @c name.
} VuoMidiInputDevice;

VuoMidiInputDevice VuoMidiInputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoMidiInputDevice_getJson(const VuoMidiInputDevice value);
char * VuoMidiInputDevice_getSummary(const VuoMidiInputDevice value);

#define VuoMidiInputDevice_SUPPORTS_COMPARISON
bool VuoMidiInputDevice_areEqual(const VuoMidiInputDevice value1, const VuoMidiInputDevice value2);
bool VuoMidiInputDevice_isLessThan(const VuoMidiInputDevice a, const VuoMidiInputDevice b);

/**
 * Automatically generated function.
 */
///@{
VuoMidiInputDevice VuoMidiInputDevice_makeFromString(const char *str);
char * VuoMidiInputDevice_getString(const VuoMidiInputDevice value);
void VuoMidiInputDevice_retain(VuoMidiInputDevice value);
void VuoMidiInputDevice_release(VuoMidiInputDevice value);
///@}

/**
 * Returns a MIDI device with the specified values.
 */
static inline VuoMidiInputDevice VuoMidiInputDevice_make(VuoInteger id, VuoText name) __attribute__((const));
static inline VuoMidiInputDevice VuoMidiInputDevice_make(VuoInteger id, VuoText name)
{
	return (VuoMidiInputDevice){id, name};
}

/**
 * @}
 */
