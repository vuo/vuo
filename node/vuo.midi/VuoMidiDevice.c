/**
 * @file
 * VuoMidiDevice implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMidiDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "MIDI Device",
					 "description" : "A set of specifications for choosing a MIDI device.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoMidiDevice
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "id" : -1,
 *     "name" : "",
 *     "isInput" : true
 *   }
 * }
 */
VuoMidiDevice VuoMidiDevice_valueFromJson(json_object * js)
{
	VuoMidiDevice md = {-1,"",true};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		md.id = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "name", &o))
		md.name = VuoText_valueFromJson(o);

	if (json_object_object_get_ex(js, "isInput", &o))
		md.isInput = VuoBoolean_valueFromJson(o);

	return md;
}

/**
 * @ingroup VuoMidiDevice
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiDevice_jsonFromValue(const VuoMidiDevice md)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_jsonFromValue(md.id);
	json_object_object_add(js, "id", idObject);

	json_object *nameObject = VuoText_jsonFromValue(md.name);
	json_object_object_add(js, "name", nameObject);

	json_object *isInputObject = VuoBoolean_jsonFromValue(md.isInput);
	json_object_object_add(js, "isInput", isInputObject);

	return js;
}

/**
 * @ingroup VuoMidiDevice
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoMidiDevice_summaryFromValue(const VuoMidiDevice md)
{
	const char *inOut = md.isInput ? "input" : "output";

	if (md.id == -1 && strlen(md.name) == 0)
		return VuoText_format("The first MIDI %s device", inOut);
	else if (md.id == -1)
		return VuoText_format("The first MIDI %s device whose name contains \"%s\"", inOut, md.name);
	else if (strlen(md.name) == 0)
		return VuoText_format("MIDI %s device #%ld", inOut, md.id);
	else
		return VuoText_format("MIDI %s device #%ld (\"%s\")", inOut, md.id, md.name);
}

/**
 * @ingroup VuoMidiDevice
 * Returns true if the two MIDI device specifications are identical.
 */
bool VuoMidiDevice_areEqual(const VuoMidiDevice value1, const VuoMidiDevice value2)
{
	return (value1.id == value2.id &&
			VuoText_areEqual(value1.name, value2.name) &&
			value1.isInput == value2.isInput);
}
