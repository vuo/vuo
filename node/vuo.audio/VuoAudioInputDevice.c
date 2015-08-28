/**
 * @file
 * VuoAudioInputDevice implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
						  "c"
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
VuoAudioInputDevice VuoAudioInputDevice_valueFromJson(json_object *js)
{
	VuoAudioInputDevice value = {-1,"",0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		value.id = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_valueFromJson(o);

	if (json_object_object_get_ex(js, "channelCount", &o))
		value.channelCount = VuoInteger_valueFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioInputDevice_jsonFromValue(const VuoAudioInputDevice value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_jsonFromValue(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *nameObject = VuoText_jsonFromValue(value.name);
	json_object_object_add(js, "name", nameObject);

	json_object *channelCountObject = VuoInteger_jsonFromValue(value.channelCount);
	json_object_object_add(js, "channelCount", channelCountObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoAudioInputDevice_summaryFromValue(const VuoAudioInputDevice value)
{
	if (value.id == -1 && strlen(value.name) == 0)
		return strdup("The default audio input device");
	else if (value.id == -1)
		return VuoText_format("The first audio input device whose name contains \"%s\"", value.name);
	else if (strlen(value.name) == 0)
		return VuoText_format("Audio input device #%ld", value.id);
	else
		// An actual detected audio input device (rather than abstract criteria).
		return VuoText_format("Audio input device #%ld (\"%s\")<br>%ld input channels", value.id, value.name, value.channelCount);
}

/**
 * Returns true if the two audio input device specifications are identical.
 */
bool VuoAudioInputDevice_areEqual(VuoAudioInputDevice value1, VuoAudioInputDevice value2)
{
	return (value1.id == value2.id &&
			VuoText_areEqual(value1.name, value2.name) &&
			value1.channelCount == value2.channelCount);
}
