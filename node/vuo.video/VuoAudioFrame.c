/**
 * @file
 * VuoAudioFrame implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAudioFrame.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Video Frame",
					  "description" : "VuoAudioSamples and timestamp for a single frame of audio.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoAudioSamples",
						"VuoList_VuoAudioSamples",
						"VuoReal"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "samples" : NULL,
 *     "timestamp" : 0.0
 *   }
 * }
 */
VuoAudioFrame VuoAudioFrame_makeFromJson(json_object *js)
{
	VuoAudioFrame value = {NULL, 0.0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "samples", &o))
		value.samples = VuoList_VuoAudioSamples_makeFromJson(o);
	else
		value.samples = NULL;

	if (json_object_object_get_ex(js, "timestamp", &o))
		value.timestamp = VuoReal_makeFromJson(o);
	else
		value.timestamp = 0.0;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioFrame_getJson(const VuoAudioFrame value)
{
	json_object *js = json_object_new_object();

	json_object *samplesObject = VuoList_VuoAudioSamples_getJson(value.samples);
	json_object_object_add(js, "samples", samplesObject);

	json_object *timestampObject = VuoReal_getJson(value.timestamp);
	json_object_object_add(js, "timestamp", timestampObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoAudioFrame_getSummary(const VuoAudioFrame value)
{
	return VuoText_format("%s<br />Timestamp: %f", VuoList_VuoAudioSamples_getSummary(value.samples), value.timestamp );
}

/**
 * Returns true if the frames have matching timestamps within a tolerance of 1 millisecond.
 */
bool VuoAudioFrame_areEqual(VuoAudioFrame value1, VuoAudioFrame value2)
{
	// Maybe this should just check image?
	return abs(value1.timestamp - value2.timestamp) < .001;
}
