/**
 * @file
 * VuoMidiNote C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoMidiNote VuoMidiNote
 * A music note event sent via MIDI.
 *
 * @{
 */

/**
 * A music note event sent via MIDI.
 */
typedef struct
{
	unsigned char channel;	///< Permitted values: 1 through 16
	bool isNoteOn;	///< Is this a Note On or Note Off event?
	unsigned char velocity;	///< Permitted values: 0 through 127
	unsigned char noteNumber;	///< Permitted values: 0 through 127
} VuoMidiNote;

VuoMidiNote VuoMidiNote_makeFromJson(struct json_object * js);
struct json_object * VuoMidiNote_getJson(const VuoMidiNote value);
char * VuoMidiNote_getSummary(const VuoMidiNote value);

#define VuoMidiNote_SUPPORTS_COMPARISON
bool VuoMidiNote_areEqual(const VuoMidiNote value1, const VuoMidiNote value2);
bool VuoMidiNote_isLessThan(const VuoMidiNote a, const VuoMidiNote b);

/**
 * Returns a note event with the specified values.
 */
static inline VuoMidiNote VuoMidiNote_make(unsigned char channel, bool isNoteOn, unsigned char velocity, unsigned char noteNumber) __attribute__((const));
static inline VuoMidiNote VuoMidiNote_make(unsigned char channel, bool isNoteOn, unsigned char velocity, unsigned char noteNumber)
{
	VuoMidiNote mn;
	mn.channel = channel;
	mn.isNoteOn = isNoteOn;
	mn.velocity = velocity;
	mn.noteNumber = noteNumber;
	return mn;
}

/// @{
/**
 * Automatically generated function.
 */
char * VuoMidiNote_getString(const VuoMidiNote value);
void VuoMidiNote_retain(VuoMidiNote value);
void VuoMidiNote_release(VuoMidiNote value);
/// @}

/**
 * @}
 */
