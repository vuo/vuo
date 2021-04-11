/**
 * @file
 * vuo.type.list.audio.real node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Audio to Real List",
					  "description": "Creates a list of real numbers using the audio sample values.",
					  "version": "1.0.1"
				 });

void nodeEvent
(
	VuoInputData(VuoAudioSamples) samples,
	VuoOutputData(VuoList_VuoReal) reals
)
{
	unsigned long count = samples.sampleCount;
	*reals = VuoListCreateWithCount_VuoReal(count, 0);
	VuoReal *outputs = VuoListGetData_VuoReal(*reals);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = samples.samples[i];
}
