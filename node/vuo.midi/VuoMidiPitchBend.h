/**
 * @file
 * VuoMidiPitchBend C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoMidiPitchBend VuoMidiPitchBend
 * A pitch bend event sent via MIDI.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"

/**
 * A pitch bend event sent via MIDI.
 */
typedef struct
{
	unsigned char channel;	///< Permitted values: 1 through 16
	unsigned short value;	///< Permitted values: 0 through 16383; center=8192
} VuoMidiPitchBend;

#define VuoMidiPitchBend_SUPPORTS_COMPARISON

VuoMidiPitchBend VuoMidiPitchBend_makeFromJson(struct json_object *js);
struct json_object *VuoMidiPitchBend_getJson(const VuoMidiPitchBend value);
char *VuoMidiPitchBend_getSummary(const VuoMidiPitchBend value);

bool VuoMidiPitchBend_areEqual(const VuoMidiPitchBend valueA, const VuoMidiPitchBend valueB);
bool VuoMidiPitchBend_isLessThan(const VuoMidiPitchBend valueA, const VuoMidiPitchBend valueB);

VuoMidiPitchBend VuoMidiPitchBend_make(VuoInteger channel, VuoInteger value);

/**
 * Automatically generated function.
 */
///@{
char *VuoMidiPitchBend_getString(const VuoMidiPitchBend value);
void VuoMidiPitchBend_retain(VuoMidiPitchBend value);
void VuoMidiPitchBend_release(VuoMidiPitchBend value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
