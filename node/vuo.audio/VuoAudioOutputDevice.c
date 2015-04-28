/**
 * @file
 * VuoAudioOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
VuoAudioOutputDevice VuoAudioOutputDevice_valueFromJson(json_object *js)
{
	VuoAudioOutputDevice value = {-1,"",0};
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
json_object * VuoAudioOutputDevice_jsonFromValue(const VuoAudioOutputDevice value)
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
char * VuoAudioOutputDevice_summaryFromValue(const VuoAudioOutputDevice value)
{
	if (value.id == -1 && strlen(value.name) == 0)
		return strdup("The default audio output device");
	else if (value.id == -1)
	{
		const char *format = "The first audio output device whose name contains \"%s\"";
		int size = snprintf(NULL,0,format,value.name);
		char *valueAsString = (char *)malloc(size+1);
		snprintf(valueAsString,size+1,format,value.name);
		return valueAsString;
	}
	else if (strlen(value.name) == 0)
	{
		const char *format = "Audio output device #%d";
		int size = snprintf(NULL,0,format,value.id);
		char *valueAsString = (char *)malloc(size+1);
		snprintf(valueAsString,size+1,format,value.id);
		return valueAsString;
	}
	else
	{
		// An actual detected audio output device (rather than abstract criteria).
		const char *format = "Audio output device #%d (\"%s\")<br>%d output channels";
		int size = snprintf(NULL,0,format,value.id,value.name,value.channelCount);
		char *valueAsString = (char *)malloc(size+1);
		snprintf(valueAsString,size+1,format,value.id,value.name,value.channelCount);
		return valueAsString;
	}
}
