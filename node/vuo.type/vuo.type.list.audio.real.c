/**
 * @file
 * vuo.type.list.audio.real node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Audio to Real List",
					  "description": "Creates a list of real numbers using the audio sample values.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoAudioSamples) samples,
	VuoOutputData(VuoList_VuoReal) reals
)
{
	*reals = VuoListCreate_VuoReal();
	unsigned long count = samples.sampleCount;

	for (unsigned long i = 0; i < count; ++i)
		VuoListAppendValue_VuoReal(*reals, samples.samples[i]);
}
