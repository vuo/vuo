/**
 * @file
 * VuoMidiInputDevice implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
	return (VuoMidiInputDevice){
		VuoJson_getObjectValue(VuoInteger, js, "id",   -1),
		VuoJson_getObjectValue(VuoText,    js, "name", NULL),
	};
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiInputDevice_getJson(const VuoMidiInputDevice md)
{
	json_object *js = json_object_new_object();

	if (md.id > 0)
	{
		json_object *idObject = VuoInteger_getJson(md.id);
		json_object_object_add(js, "id", idObject);
	}

	if (md.name)
	{
		json_object *nameObject = VuoText_getJson(md.name);
		json_object_object_add(js, "name", nameObject);
	}

	return js;
}

/**
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoMidiInputDevice_getSummary(const VuoMidiInputDevice md)
{
	if (md.id == -1 && VuoText_isEmpty(md.name))
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

/**
 * Returns true if the id of `a` is less than the id of `b`.
 * @version200New
 */
bool VuoMidiInputDevice_isLessThan(const VuoMidiInputDevice a, const VuoMidiInputDevice b)
{
	return a.id < b.id;
}
