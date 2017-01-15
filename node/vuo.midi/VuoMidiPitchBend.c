/**
 * @file
 * VuoMidiPitchBend implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMidiPitchBend.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "MIDI Pitch Bend",
					  "description" : "A pitch bend event sent via MIDI.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoInteger",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "channel" : 1,
 *     "value" : 8192
 *   }
 * }
 */
VuoMidiPitchBend VuoMidiPitchBend_makeFromJson(json_object *js)
{
	VuoMidiPitchBend value = {1, 8192};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "channel", &o))
		value.channel = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "value", &o))
		value.value = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoMidiPitchBend_getJson(const VuoMidiPitchBend value)
{
	json_object *js = json_object_new_object();

	json_object *channelObject = VuoInteger_getJson(value.channel);
	json_object_object_add(js, "channel", channelObject);

	json_object *valueObject = VuoInteger_getJson(value.value);
	json_object_object_add(js, "value", valueObject);

	return js;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoMidiPitchBend_areEqual(const VuoMidiPitchBend valueA, const VuoMidiPitchBend valueB)
{
	return valueA.channel == valueB.channel
		&& valueA.value == valueB.value;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoMidiPitchBend_isLessThan(const VuoMidiPitchBend valueA, const VuoMidiPitchBend valueB)
{
	return (VuoInteger)(valueA.channel << 14) + valueA.value < (VuoInteger)(valueB.channel << 14) + valueB.value;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoMidiPitchBend_getSummary(const VuoMidiPitchBend value)
{
	return VuoText_format("Channel %d: value %d",
						  value.channel, value.value);
}

/**
 * Returns a MIDI Pitch Bend with the specified values.
 */
VuoMidiPitchBend VuoMidiPitchBend_make(VuoInteger channel, VuoInteger value)
{
	return (VuoMidiPitchBend){channel, value};
}

