/**
 * @file
 * VuoAudioInputDevice implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
	return (VuoAudioInputDevice){
		VuoJson_getObjectValue(VuoInteger, js, "id",           -1),
		VuoJson_getObjectValue(VuoText,    js, "modelUid",     NULL),
		VuoJson_getObjectValue(VuoText,    js, "name",         NULL),
		VuoJson_getObjectValue(VuoInteger, js, "channelCount", 0),
	};
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioInputDevice_getJson(const VuoAudioInputDevice value)
{
	json_object *js = json_object_new_object();

	if (value.id != -1)
	{
		json_object *idObject = VuoInteger_getJson(value.id);
		json_object_object_add(js, "id", idObject);
	}

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

	if (value.channelCount)
	{
		json_object *channelCountObject = VuoInteger_getJson(value.channelCount);
		json_object_object_add(js, "channelCount", channelCountObject);
	}

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
 * Returns a compact string representation of @c value.
 */
char * VuoAudioInputDevice_getShortSummary(const VuoAudioInputDevice value)
{
	if (value.id == -1)
	{
		if (VuoText_isEmpty(value.modelUid))
		{
			if (VuoText_isEmpty(value.name))
				return strdup("Default");
			else // have name
				return strdup(value.name);
		}
		else // have model
		{
			if (VuoText_isEmpty(value.name))
				return strdup(value.modelUid);
			else // have name
				return strdup(value.name);
		}
	}
	else // have ID
	{
		if (VuoText_isEmpty(value.name))
			return VuoText_format("Device #%lld", value.id);
		else
			// An actual detected audio input device (rather than abstract criteria).
			return strdup(value.name);
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

/**
 * Returns true if the id of `a` is less than the id of `b`.
 * @version200New
 */
bool VuoAudioInputDevice_isLessThan(const VuoAudioInputDevice a, const VuoAudioInputDevice b)
{
	return a.id < b.id;
}
