/**
 * @file
 * vuo.audio.mix node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					 "title" : "Mix Audio Channels",
					 "keywords" : [ "sound", "music", "merge", "combine" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoAudio"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoAudioSamples) audioSamples,
		VuoOutputData(VuoAudioSamples) mixedSamples
)
{
	unsigned int channelCount = VuoListGetCount_VuoAudioSamples(audioSamples);

	*mixedSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
		
	for(unsigned int n = 0; n < (*mixedSamples).sampleCount; n++)
		(*mixedSamples).samples[n] = 0.;

	for(unsigned int i = 0; i < channelCount; i++)
	{
		VuoAudioSamples as = VuoListGetValueAtIndex_VuoAudioSamples(audioSamples, i+1);

		for(unsigned int n = 0; n < as.sampleCount; n++)
			(*mixedSamples).samples[n] += as.samples[n];
	}
}
