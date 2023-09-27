/**
 * @file
 * VuoMidiOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
VuoMidiOutputDevice VuoMidiOutputDevice_makeFromJson(json_object * js)
{
	return (VuoMidiOutputDevice){
		VuoJson_getObjectValue(VuoInteger, js, "id",   -1),
		VuoJson_getObjectValue(VuoText,    js, "name", NULL),
	};
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiOutputDevice_getJson(const VuoMidiOutputDevice md)
{
	json_object *js = json_object_new_object();

	if (md.id > 0)
	{
		json_object *idObject = VuoInteger_getJson(md.id);
		json_object_object_add(js, "id", idObject);
	}

	if  (md.name)
	{
		json_object *nameObject = VuoText_getJson(md.name);
		json_object_object_add(js, "name", nameObject);
	}

	return js;
}

/**
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoMidiOutputDevice_getSummary(const VuoMidiOutputDevice md)
{
	if (md.id == -1 && VuoText_isEmpty(md.name))
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

/**
 * Returns true if the id of `a` is less than the id of `b`.
 */
bool VuoMidiOutputDevice_isLessThan(const VuoMidiOutputDevice a, const VuoMidiOutputDevice b)
{
	return a.id < b.id;
}

/**
 * `VuoCompilerType::parseOrGenerateRetainOrReleaseFunction` can't currently generate this on arm64.
 * https://b33p.net/kosada/vuo/vuo/-/issues/19142#note_2158967
 */
void VuoMidiOutputDevice_retain(VuoMidiOutputDevice value)
{
	VuoRetain(value.name);
}

/**
 * `VuoCompilerType::parseOrGenerateRetainOrReleaseFunction` can't currently generate this on arm64.
 * https://b33p.net/kosada/vuo/vuo/-/issues/19142#note_2158967
 */
void VuoMidiOutputDevice_release(VuoMidiOutputDevice value)
{
	VuoRelease(value.name);
}
