/**
 * @file
 * vuo.audio.mix node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Mix Audio Channels",
					 "keywords" : [ "sound", "music", "merge", "combine" ],
					 "version" : "2.0.0",
					 "node": {
						 "exampleCompositions" : [ "PanAudio.vuo" ],
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoAudioSamples) samples,
		VuoOutputData(VuoAudioSamples) mixedSamples
)
{
	unsigned int channelCount = VuoListGetCount_VuoAudioSamples(samples);

	*mixedSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);

	(*mixedSamples).samplesPerSecond = VuoAudioSamples_sampleRate;

	for(unsigned int n = 0; n < (*mixedSamples).sampleCount; n++)
		(*mixedSamples).samples[n] = 0.;

	for(unsigned int i = 0; i < channelCount; i++)
	{
		VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(samples, i+1);

		for(unsigned int n = 0; n < as.sampleCount; n++)
			(*mixedSamples).samples[n] += as.samples[n];
	}
}
