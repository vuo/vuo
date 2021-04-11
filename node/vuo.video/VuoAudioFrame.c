/**
 * @file
 * VuoAudioFrame implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoAudioFrame.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Audio Frame",
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
	return (VuoAudioFrame){
		VuoJson_getObjectValue(VuoList_VuoAudioSamples, js, "channels",  NULL),
		VuoJson_getObjectValue(VuoReal,                 js, "timestamp", 0),
	};
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioFrame_getJson(const VuoAudioFrame value)
{
	json_object *js = json_object_new_object();

	json_object *channelsObject = VuoList_VuoAudioSamples_getJson(value.channels);
	json_object_object_add(js, "channels", channelsObject);

	json_object *timestampObject = VuoReal_getJson(value.timestamp);
	json_object_object_add(js, "timestamp", timestampObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoAudioFrame_getSummary(const VuoAudioFrame value)
{
	return VuoText_format("%s<br />Timestamp: <tt>%.3f</tt> seconds", VuoList_VuoAudioSamples_getSummary(value.channels), value.timestamp );
}

/**
 * Returns true if the frames have matching timestamps within a tolerance of 1 millisecond.
 */
bool VuoAudioFrame_areEqual(VuoAudioFrame value1, VuoAudioFrame value2)
{
    return fabs(value1.timestamp - value2.timestamp) < .001;
}

/**
 * Returns true if the timestamp of `a` is less than the timestamp of `b`.
 * @version200New
 */
bool VuoAudioFrame_isLessThan(const VuoAudioFrame a, const VuoAudioFrame b)
{
	return a.timestamp < b.timestamp;
}
