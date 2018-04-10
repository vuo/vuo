/**
 * @file
 * VuoAudioInputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAudioInputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Audio Input Device",
					  "description" : "Information about an audio input device.",
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
VuoAudioInputDevice VuoAudioInputDevice_makeFromJson(json_object *js)
{
	VuoAudioInputDevice value = {-1, NULL, NULL, 0};
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
json_object * VuoAudioInputDevice_getJson(const VuoAudioInputDevice value)
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
char * VuoAudioInputDevice_getSummary(const VuoAudioInputDevice value)
{
	if (value.id == -1)
	{
		if (VuoText_isEmpty(value.modelUid))
		{
			if (VuoText_isEmpty(value.name))
				return strdup("The default audio input device");
			else // have name
				return VuoText_format("The first audio input device whose name contains \"%s\"", value.name);
		}
		else // have model
		{
			if (VuoText_isEmpty(value.name))
				return VuoText_format("The first audio input device whose model contains \"%s\"", value.modelUid);
			else // have name
				return VuoText_format("The first audio input device of the same model as \"%s\"", value.name);
		}
	}
	else // have ID
	{
		if (VuoText_isEmpty(value.name))
			return VuoText_format("Audio input device #%lld", value.id);
		else
			// An actual detected audio input device (rather than abstract criteria).
			return VuoText_format("Audio input device #%lld<br>Name: \"%s\"<br>Model: \"%s\"<br>%lld input channels", value.id, value.name, value.modelUid, value.channelCount);
	}
}

/**
 * Returns true if the two audio input device specifications are identical.
 */
bool VuoAudioInputDevice_areEqual(VuoAudioInputDevice value1, VuoAudioInputDevice value2)
{
	return (value1.id == value2.id &&
			VuoText_areEqual(value1.modelUid, value2.modelUid) &&
			VuoText_areEqual(value1.name, value2.name) &&
			value1.channelCount == value2.channelCount);
}
