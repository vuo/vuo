/**
 * @file
 * VuoMidiOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMidiOutputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "MIDI Output Device",
					 "description" : "A set of specifications for choosing a MIDI output device.",
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
 *     "id" : -1,
 *     "name" : ""
 *   }
 * }
 */
VuoMidiOutputDevice VuoMidiOutputDevice_valueFromJson(json_object * js)
{
	VuoMidiOutputDevice md = {-1,""};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		md.id = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "name", &o))
		md.name = VuoText_valueFromJson(o);
	else
		md.name = VuoText_make("");

	return md;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiOutputDevice_jsonFromValue(const VuoMidiOutputDevice md)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_jsonFromValue(md.id);
	json_object_object_add(js, "id", idObject);

	json_object *nameObject = VuoText_jsonFromValue(md.name);
	json_object_object_add(js, "name", nameObject);

	return js;
}

/**
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoMidiOutputDevice_summaryFromValue(const VuoMidiOutputDevice md)
{
	if (md.id == -1 && strlen(md.name) == 0)
		return VuoText_format("The first MIDI output device");
	else if (md.id == -1)
		return VuoText_format("The first MIDI output device whose name contains \"%s\"", md.name);
	else if (strlen(md.name) == 0)
		return VuoText_format("MIDI output device #%lld", md.id);
	else
		return VuoText_format("MIDI output device #%lld (\"%s\")", md.id, md.name);
}

/**
 * Returns true if the two MIDI device specifications are identical.
 */
bool VuoMidiOutputDevice_areEqual(const VuoMidiOutputDevice value1, const VuoMidiOutputDevice value2)
{
	return value1.id == value2.id
		&& VuoText_areEqual(value1.name, value2.name);
}
