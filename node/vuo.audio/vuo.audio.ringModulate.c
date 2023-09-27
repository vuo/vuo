/**
 * @file
 * vuo.audio.ringModulate node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoList_VuoAudioSamples.h"

VuoModuleMetadata({
					"title" : "Ring Modulate Audio",
					"keywords" : [
						"sound", "music", "synthesis", "filter",
						"merge", "combine", "multiply", "product", "times", "*", "•", "×", "x",
						"AM", "amplitude modulation",
					],
					"version" : "1.0.0",
					"node": {
						"exampleCompositions" : [ "CompareMixAndRingModulate.vuo" ],
					}
				});

void nodeEvent
(
	VuoInputData(VuoList_VuoAudioSamples) channels,
	VuoOutputData(VuoAudioSamples) modulatedSamples
)
{
	*modulatedSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
	(*modulatedSamples).samplesPerSecond = VuoAudioSamples_sampleRate;

	unsigned int channelCount = VuoListGetCount_VuoAudioSamples(channels);
	if (channelCount == 0)
	{
		for (unsigned int n = 0; n < VuoAudioSamples_bufferSize; n++)
			(*modulatedSamples).samples[n] = 0;
		return;
	}

	bool firstAudioPort = true;
	for (unsigned int i = 0; i < channelCount; i++)
	{
		VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(channels, i+1);

		if (VuoAudioSamples_isPopulated(as))
		{
			if (firstAudioPort)
			{
				firstAudioPort = false;
				for (unsigned int n = 0; n < as.sampleCount; n++)
					(*modulatedSamples).samples[n] = as.samples[n];
			}
			else
			{
				for (unsigned int n = 0; n < as.sampleCount; n++)
					(*modulatedSamples).samples[n] *= as.samples[n];
			}
		}
	}
}
