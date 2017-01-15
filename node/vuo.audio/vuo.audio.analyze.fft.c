/**
 * @file
 * vuo.audio.analyze.fft node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDsp.h"
#include "VuoAudioBins.h"
#include "VuoAudioBinAverageType.h"

VuoModuleMetadata({
					 "title" : "Calculate Amplitude for Frequencies",
					 "keywords" : [ "sound", "fourier", "music", "waveform", "spectrum", "pass", "filter", "signal",
						"equalizer", "equaliser" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoDsp"
					 ],
					 "node" : {
						 "exampleCompositions" : [ "ShowAudioFrequencies.vuo" ],
					 }
				 });

struct nodeInstanceData
{
	VuoDsp vdsp;
	VuoReal* sampleQueue;
	VuoAudioBins curFreqBins;
};

struct nodeInstanceData* nodeInstanceInit(
	VuoInputData(VuoAudioBins, {"default":"255"}) frequencyBins
)
{
	struct nodeInstanceData* instance = (struct nodeInstanceData*) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	unsigned int binSize = (unsigned int)frequencyBins;
	instance->sampleQueue = (VuoReal*)calloc(binSize, sizeof(VuoReal));

	instance->curFreqBins = frequencyBins;

	instance->vdsp = VuoDsp_make(binSize, VuoWindowing_Blackman);
	VuoRetain(instance->vdsp);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData*) instance,
		VuoInputData(VuoAudioSamples) samples,
		VuoInputData(VuoAudioBins, {"default":"255"}) frequencyBins,
		VuoInputData(VuoAudioBinAverageType, {"default":"quadratic"}) frequencyBinAveraging,
		VuoOutputData(VuoList_VuoReal) amplitudes
)
{
	unsigned int binSize = (unsigned int)frequencyBins;

	if(frequencyBins != (*instance)->curFreqBins)
	{
		(*instance)->curFreqBins = frequencyBins;

		// for now just drop the current bin and restart the process
		free((*instance)->sampleQueue);
		(*instance)->sampleQueue = (VuoReal*)calloc(binSize, sizeof(VuoReal));

		VuoRelease((*instance)->vdsp);
		(*instance)->vdsp = VuoDsp_make(binSize, VuoWindowing_Blackman);
		VuoRetain((*instance)->vdsp);
	}

	unsigned int freqCount;

	/**
	 * If sampleCount is greater than the number of bins, analyze and average the whole sample.
	 * Otherwise, run a rolling queue of sample data for analysis
	 */
	if(samples.sampleCount > binSize)
	{
		VuoReal* avg = (VuoReal*)calloc(binSize, sizeof(VuoReal));

		for(int i = 0; i < samples.sampleCount/binSize; ++i)
		{
			VuoReal* freq = VuoDsp_frequenciesForSamples((*instance)->vdsp, &samples.samples[(binSize*i)], binSize, frequencyBinAveraging, &freqCount);

			for(int n = 0; n < freqCount; n++)
				avg[n] += freq[n];

			free(freq);
		}

		VuoList_VuoReal frequencies = VuoListCreate_VuoReal();

		for(int i = 0; i < freqCount; i++)
			VuoListAppendValue_VuoReal(frequencies, avg[i] / (float)(samples.sampleCount/binSize));

		*amplitudes = frequencies;
		free(avg);
	}
	else
	{
		unsigned int sampleCount = samples.sampleCount;

		// move old data out of the back of the array and into the front
		memmove((*instance)->sampleQueue, (*instance)->sampleQueue + sampleCount, (binSize - sampleCount) * sizeof(VuoReal));

		// copy new data into rear of array
		memcpy((*instance)->sampleQueue + (binSize - sampleCount), samples.samples, sampleCount * sizeof(VuoReal));

		VuoReal* freq = VuoDsp_frequenciesForSamples((*instance)->vdsp, (*instance)->sampleQueue, binSize, frequencyBinAveraging, &freqCount);

		VuoList_VuoReal frequencies = VuoListCreate_VuoReal();
		for(int i = 0; i < freqCount; i++)
			VuoListAppendValue_VuoReal(frequencies, freq[i]);

		free(freq);

		*amplitudes = frequencies;
	}
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData*) instance
)
{
	VuoRelease((*instance)->vdsp);
	free((*instance)->sampleQueue);
}
