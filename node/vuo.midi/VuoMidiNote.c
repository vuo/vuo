/**
 * @file
 * VuoMidiNote implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMidiNote.h"
#include "VuoBoolean.h"
#include "VuoInteger.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "MIDI Note",
					 "description" : "A music note event sent via MIDI.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoBoolean",
						"VuoInteger",
						"VuoText"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoMidiNote
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "channel" : 1,
 *     "isNoteOn" : true,
 *     "velocity" : 127,
 *     "noteNumber" : 60
 *   }
 * }
 */
VuoMidiNote VuoMidiNote_makeFromJson(json_object * js)
{
	VuoMidiNote mn = {1,true,127,60};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "channel", &o))
		mn.channel = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "isNoteOn", &o))
		mn.isNoteOn = VuoBoolean_makeFromJson(o);

	if (json_object_object_get_ex(js, "velocity", &o))
		mn.velocity = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "noteNumber", &o))
		mn.noteNumber = VuoInteger_makeFromJson(o);

	return mn;
}

/**
 * @ingroup VuoMidiNote
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiNote_getJson(const VuoMidiNote mn)
{
	json_object *js = json_object_new_object();

	json_object *channelObject = VuoInteger_getJson(mn.channel);
	json_object_object_add(js, "channel", channelObject);

	json_object *isNoteOnObject = VuoBoolean_getJson(mn.isNoteOn);
	json_object_object_add(js, "isNoteOn", isNoteOnObject);

	json_object *velocityObject = VuoInteger_getJson(mn.velocity);
	json_object_object_add(js, "velocity", velocityObject);

	json_object *noteNumberObject = VuoInteger_getJson(mn.noteNumber);
	json_object_object_add(js, "noteNumber", noteNumberObject);

	return js;
}

/**
 * @ingroup VuoMidiNote
 * Returns a compact string representation of @c value (comma-separated coordinates).
 *
 * Includes the note name with the ASA 1939 octave designator (e.g., noteNumber 60, middle C, is "C4").
 * Journal of the Acoustical Society of America, Volume 11, Issue 1, pp. 134-139 (1939).
 */
char * VuoMidiNote_getSummary(const VuoMidiNote mn)
{
	const char *noteNames[] = { "C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B" };
	const char *noteName = noteNames[mn.noteNumber % 12];
	unsigned char noteOctave = mn.noteNumber / 12 - 1;
	const char *onOff = mn.isNoteOn ? "on" : "off";

	return VuoText_format("Channel %d: note %s%d (#%d) %s, velocity %d",
						  mn.channel, noteName, noteOctave, mn.noteNumber, onOff, mn.velocity);
}

/**
 * Returns true if the channel, note status (on/off), velocity, and note number all match.
 */
bool VuoMidiNote_areEqual(const VuoMidiNote value1, const VuoMidiNote value2)
{
	return (value1.channel    == value2.channel
		 && value1.isNoteOn   == value2.isNoteOn
		 && value1.velocity   == value2.velocity
		 && value1.noteNumber == value2.noteNumber);
}
