/**
 * @file
 * VuoMidiDevice C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMIDIDEVICE_H
#define VUOMIDIDEVICE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoMidiDevice VuoMidiDevice
 * A set of specifications for choosing a MIDI device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"
#include "VuoBoolean.h"

/**
 * A set of specifications for choosing a MIDI device.
 */
typedef struct
{
	VuoInteger id;	///< If @c id is non-negative, use the specified device identifier.
	VuoText name;	///< If @c id is negative, use the first device whose name contains @c name.
	VuoBoolean isInput;	///< Is this an input or output device?
} VuoMidiDevice;

VuoMidiDevice VuoMidiDevice_valueFromJson(struct json_object * js);
struct json_object * VuoMidiDevice_jsonFromValue(const VuoMidiDevice value);
char * VuoMidiDevice_summaryFromValue(const VuoMidiDevice value);
bool VuoMidiDevice_areEqual(const VuoMidiDevice value1, const VuoMidiDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoMidiDevice VuoMidiDevice_valueFromString(const char *str);
char * VuoMidiDevice_stringFromValue(const VuoMidiDevice value);
void VuoMidiDevice_retain(VuoMidiDevice value);
void VuoMidiDevice_release(VuoMidiDevice value);
///@}

/**
 * Returns a MIDI device with the specified values.
 */
static inline VuoMidiDevice VuoMidiDevice_make(VuoInteger id, VuoText name, VuoBoolean isInput) __attribute__((const));
static inline VuoMidiDevice VuoMidiDevice_make(VuoInteger id, VuoText name, VuoBoolean isInput)
{
	VuoMidiDevice md = {id,name,isInput};
	return md;
}

/**
 * @}
 */

#endif
