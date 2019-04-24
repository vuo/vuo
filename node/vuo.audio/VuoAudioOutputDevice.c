/**
 * @file
 * VuoAudioOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAudioOutputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Audio Output Device",
					  "description" : "Information about an audio output device.",
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
 *     "modelUid" : "",
 *     "name" : ""
 *   }
 * }
 */
VuoAudioOutputDevice VuoAudioOutputDevice_makeFromJson(json_object *js)
{
	VuoAudioOutputDevice value = {-1, NULL, NULL, 0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		value.id = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "modelUid", &o))
		value.modelUid = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "channelCount", &o))
		value.channelCount = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioOutputDevice_getJson(const VuoAudioOutputDevice value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_getJson(value.id);
	json_object_object_add(js, "id", idObject);

	if (value.modelUid)
	{
		json_object *nameObject = VuoText_getJson(value.modelUid);
		json_object_object_add(js, "modelUid", nameObject);
	}

	if (value.name)
	{
		json_object *nameObject = VuoText_getJson(value.name);
		json_object_object_add(js, "name", nameObject);
	}

	json_object *channelCountObject = VuoInteger_getJson(value.channelCount);
	json_object_object_add(js, "channelCount", channelCountObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoAudioOutputDevice_getSummary(const VuoAudioOutputDevice value)
{
	if (value.id == -1)
	{
		if (VuoText_isEmpty(value.modelUid))
		{
			if (VuoText_isEmpty(value.name))
				return strdup("The default audio output device");
			else // have name
				return VuoText_format("The first audio output device whose name contains \"%s\"", value.name);
		}
		else // have model
		{
			if (VuoText_isEmpty(value.name))
				return VuoText_format("The first audio output device whose model contains \"%s\"", value.modelUid);
			else // have name
				return VuoText_format("The first audio output device of the same model as \"%s\"", value.name);
		}
	}
	else // have ID
	{
		if (VuoText_isEmpty(value.name))
			return VuoText_format("Audio output device #%lld", value.id);
		else
			// An actual detected audio output device (rather than abstract criteria).
			return VuoText_format("Audio output device #%lld<br>Name: \"%s\"<br>Model: \"%s\"<br>%lld output channels", value.id, value.name, value.modelUid, value.channelCount);
	}
}

/**
 * Returns true if the two audio output device specifications are identical.
 */
bool VuoAudioOutputDevice_areEqual(VuoAudioOutputDevice value1, VuoAudioOutputDevice value2)
{
	return (value1.id == value2.id &&
			VuoText_areEqual(value1.modelUid, value2.modelUid) &&
			VuoText_areEqual(value1.name, value2.name) &&
			value1.channelCount == value2.channelCount);
}
