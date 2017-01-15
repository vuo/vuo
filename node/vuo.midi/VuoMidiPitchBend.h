/**
 * @file
 * VuoMidiPitchBend C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMIDIPITCHBEND_H
#define VUOMIDIPITCHBEND_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoMidiPitchBend VuoMidiPitchBend
 * A pitch bend event sent via MIDI.
 *
 * @{
 */

#include "VuoInteger.h"

/**
 * A pitch bend event sent via MIDI.
 */
typedef struct
{
	unsigned char channel;	///< Permitted values: 1 through 16
	unsigned short value;	///< Permitted values: 0 through 16383; center=8192

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoMidiPitchBend;

VuoMidiPitchBend VuoMidiPitchBend_makeFromJson(struct json_object *js);
struct json_object *VuoMidiPitchBend_getJson(const VuoMidiPitchBend value);
char *VuoMidiPitchBend_getSummary(const VuoMidiPitchBend value);

#define VuoMidiPitchBend_SUPPORTS_COMPARISON
bool VuoMidiPitchBend_areEqual(const VuoMidiPitchBend valueA, const VuoMidiPitchBend valueB);
bool VuoMidiPitchBend_isLessThan(const VuoMidiPitchBend valueA, const VuoMidiPitchBend valueB);

VuoMidiPitchBend VuoMidiPitchBend_make(VuoInteger channel, VuoInteger value);

/**
 * Automatically generated function.
 */
///@{
VuoMidiPitchBend VuoMidiPitchBend_makeFromString(const char *str);
char *VuoMidiPitchBend_getString(const VuoMidiPitchBend value);
void VuoMidiPitchBend_retain(VuoMidiPitchBend value);
void VuoMidiPitchBend_release(VuoMidiPitchBend value);
///@}

/**
 * @}
 */

#endif // VUOMIDIPITCHBEND_H

