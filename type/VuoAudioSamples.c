/**
 * @file
 * VuoAudioSamples implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAudioSamples.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Audio Samples",
					  "description" : "A set of audio amplitudes for a single channel.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoInteger",
						"VuoReal",
						"VuoText"
					  ]
				  });
#endif
/// @}

const VuoInteger VuoAudioSamples_bufferSize = 512;	///<https://b33p.net/kosada/node/7650
const VuoReal VuoAudioSamples_sampleRate = 48000;	///<https://b33p.net/kosada/node/7649

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "samples" : [ -0.014525, 0.015363, 0.013679 ],
 *     "samplesPerSecond" : 44100.000000
 *   }
 * }
 */
VuoAudioSamples VuoAudioSamples_makeFromJson(json_object * js)
{
	VuoAudioSamples value = {0, NULL, 0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "samples", &o))
	{
		int sampleCount = json_object_array_length(o);
		if (sampleCount)
		{
			value = VuoAudioSamples_alloc(sampleCount);

			for (VuoInteger i = 0; i < sampleCount; ++i)
				value.samples[i] = json_object_get_double(json_object_array_get_idx(o, i));
		}
		else
		{
			value.sampleCount = 0;
			value.samples = NULL;
		}
	}

	if (json_object_object_get_ex(js, "samplesPerSecond", &o))
		value.samplesPerSecond = VuoReal_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioSamples_getJson(const VuoAudioSamples value)
{
	json_object *js = json_object_new_object();

	json_object *samplesObject = json_object_new_array();
	for (VuoInteger i=0; i<value.sampleCount; ++i)
		json_object_array_add(samplesObject, VuoReal_getJson(value.samples[i]));
	json_object_object_add(js, "samples", samplesObject);

	json_object *samplesPerSecondObject = VuoReal_getJson(value.samplesPerSecond);
	json_object_object_add(js, "samplesPerSecond", samplesPerSecondObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoAudioSamples_getSummary(const VuoAudioSamples value)
{
	return VuoText_format("%lld samples @ %g kHz", value.sampleCount, value.samplesPerSecond/1000);
}

/**
 * Allocates and registers the @c samples array.
 */
VuoAudioSamples VuoAudioSamples_alloc(VuoInteger sampleCount)
{
	VuoAudioSamples value;

	value.sampleCount = sampleCount;

	value.samples = (VuoReal *)malloc(sizeof(VuoReal)*value.sampleCount);
	VuoRegister(value.samples, free);

	return value;
}

/**
 * - If there are no audio samples, returns true.
 * - If all audio samples have amplitude 0, returns true.
 * - Otherwise returns false.
 */
bool VuoAudioSamples_isEmpty(const VuoAudioSamples samples)
{
	if (samples.sampleCount == 0)
		return true;

	for (VuoInteger i = 0; i < samples.sampleCount; ++i)
		if (fabs(samples.samples[i]) >= 0.0001)
			return false;

	return true;
}

/**
 * - If there are no audio samples, returns false.
 * - Otherwise returns true.
 */
bool VuoAudioSamples_isPopulated(const VuoAudioSamples samples)
{
	return (samples.sampleCount > 0);
}
