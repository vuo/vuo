/**
 * @file
 * VuoMidiOutputDevice C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMIDIOUTPUTDEVICE_H
#define VUOMIDIOUTPUTDEVICE_H

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

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoMidiOutputDevice;

VuoMidiOutputDevice VuoMidiOutputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoMidiOutputDevice_getJson(const VuoMidiOutputDevice value);
char * VuoMidiOutputDevice_getSummary(const VuoMidiOutputDevice value);
bool VuoMidiOutputDevice_areEqual(const VuoMidiOutputDevice value1, const VuoMidiOutputDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoMidiOutputDevice VuoMidiOutputDevice_makeFromString(const char *str);
char * VuoMidiOutputDevice_getString(const VuoMidiOutputDevice value);
void VuoMidiOutputDevice_retain(VuoMidiOutputDevice value);
void VuoMidiOutputDevice_release(VuoMidiOutputDevice value);
///@}

/**
 * Returns a MIDI device with the specified values.
 */
static inline VuoMidiOutputDevice VuoMidiOutputDevice_make(VuoInteger id, VuoText name) __attribute__((const));
static inline VuoMidiOutputDevice VuoMidiOutputDevice_make(VuoInteger id, VuoText name)
{
	VuoMidiOutputDevice md = {id,name};
	return md;
}

/**
 * @}
 */

#endif // VUOMIDIOUTPUTDEVICE_H
