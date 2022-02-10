/**
 * @file
 * VuoAudioSamples C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoAudioSamples VuoAudioSamples
 * A set of audio amplitudes for a single channel.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoReal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const VuoInteger VuoAudioSamples_bufferSize;	///<https://b33p.net/kosada/node/7650
extern const VuoReal VuoAudioSamples_sampleRate;	///<https://b33p.net/kosada/node/7649

/**
 * A set of audio amplitudes for a single channel.
 */
typedef struct
{
	VuoInteger sampleCount;		///< The number of elements in @c samples.
	VuoReal *samples;			///< Samples should typically be between -1 and 1, centered at 0.

	VuoReal samplesPerSecond;	///< The sample rate.  E.g., 48000.
} VuoAudioSamples;

VuoAudioSamples VuoAudioSamples_makeFromJson(struct json_object * js);
struct json_object * VuoAudioSamples_getJson(const VuoAudioSamples value);
char * VuoAudioSamples_getSummary(const VuoAudioSamples value);

VuoAudioSamples VuoAudioSamples_alloc(VuoInteger sampleCount);

bool VuoAudioSamples_isEmpty(const VuoAudioSamples samples);
bool VuoAudioSamples_isPopulated(const VuoAudioSamples samples);

/**
 * Automatically generated function.
 */
///@{
char * VuoAudioSamples_getString(const VuoAudioSamples value);
void VuoAudioSamples_retain(VuoAudioSamples value);
void VuoAudioSamples_release(VuoAudioSamples value);
///@}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
