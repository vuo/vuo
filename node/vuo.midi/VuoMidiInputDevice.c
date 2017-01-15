/**
 * @file
 * VuoMidiInputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMidiInputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "MIDI Input Device",
					 "description" : "A set of specifications for choosing a MIDI input device.",
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
VuoMidiInputDevice VuoMidiInputDevice_makeFromJson(json_object * js)
{
	VuoMidiInputDevice md = {-1,""};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		md.id = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "name", &o))
		md.name = VuoText_makeFromJson(o);
	else
		md.name = VuoText_make("");

	return md;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiInputDevice_getJson(const VuoMidiInputDevice md)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_getJson(md.id);
	json_object_object_add(js, "id", idObject);

	json_object *nameObject = VuoText_getJson(md.name);
	json_object_object_add(js, "name", nameObject);

	return js;
}

/**
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoMidiInputDevice_getSummary(const VuoMidiInputDevice md)
{
	if (md.id == -1 && (!md.name || md.name[0] == 0))
		return VuoText_format("The first MIDI input device");
	else if (md.id == -1)
		return VuoText_format("The first MIDI input device whose name contains \"%s\"", md.name);
	else if (strlen(md.name) == 0)
		return VuoText_format("MIDI input device #%lld", md.id);
	else
		return VuoText_format("MIDI input device #%lld (\"%s\")", md.id, md.name);
}

/**
 * Returns true if the two MIDI device specifications are identical.
 */
bool VuoMidiInputDevice_areEqual(const VuoMidiInputDevice value1, const VuoMidiInputDevice value2)
{
	return value1.id == value2.id
		&& VuoText_areEqual(value1.name, value2.name);
}
