/**
 * @file
 * vuo.audio.split.frequency node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "node.h"
#include "VuoWave.h"
#include "VuoAudioSamples.h"
#include "math.h"

VuoModuleMetadata({
					 "title" : "Split Audio by Frequency",
					 "keywords" : [ "processor" ],
					 "version" : "2.0.1",
					 "dependencies" : [
						 "Gamma"
					 ],
					 "node": {
						 "exampleCompositions" : [ "VisualizeFrequencies.vuo" ]
					 }
				 });

}

#include "Gamma/Filter.h"
#include "Gamma/Sync.h"

#define PI 3.14159265359

struct nodeInstanceData
{
	gam::Sync *sync;
	gam::Biquad<> *biquad_low;
	gam::Biquad<> *biquad_band;
	gam::Biquad<> *biquad_high;
};

extern "C" struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sync = new gam::Sync(VuoAudioSamples_sampleRate);

	// second argument is resonance
	context->biquad_low = new gam::Biquad<>(1000, sqrt(2), gam::LOW_PASS);
	context->biquad_band = new gam::Biquad<>(1000, sqrt(2), gam::BAND_PASS);
	context->biquad_high = new gam::Biquad<>(1000, sqrt(2), gam::HIGH_PASS);

	*(context->sync) << *(context->biquad_low);
	*(context->sync) << *(context->biquad_band);
	*(context->sync) << *(context->biquad_high);

	return context;
}

static int compare (const void * a, const void * b)
{
	double x = *(double*)a;
	double y = *(double*)b;
	return x-y;
}

static VuoAudioSamples SamplesWithFilterAndFrequency(const VuoAudioSamples samples, gam::Biquad<> *filter, const double frq)
{
	(*filter).freq(frq);

	VuoAudioSamples filteredSamples = VuoAudioSamples_alloc(samples.sampleCount);
	unsigned int sampleCount = samples.sampleCount;
	filteredSamples.samplesPerSecond = samples.samplesPerSecond;

	if( (*filter).type() == gam::BAND_PASS)
	{
		for(int i = 0; i < sampleCount; i++)
			filteredSamples.samples[i] = (*filter).nextBP( samples.samples[i] );
	}
	else
	{
		for(int i = 0; i < sampleCount; i++)
			filteredSamples.samples[i] = (*filter)( samples.samples[i] );
	}

	return filteredSamples;
}

extern "C" void nodeInstanceEvent
(
	VuoInputData(VuoAudioSamples) samples,
	VuoInputData(VuoList_VuoReal, {"default":[300, 4000]}) cutoffFrequencies,
	VuoInputEvent({"data":"cutoffFrequencies", "eventBlocking":"wall"}) cutoffFrequenciesEvent,
	VuoOutputData(VuoList_VuoAudioSamples) splitSamples,
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	unsigned int len = VuoListGetCount_VuoReal(cutoffFrequencies);

	if(len < 1) return;

	*splitSamples = VuoListCreate_VuoAudioSamples();

	(*context)->sync->spu(VuoAudioSamples_sampleRate);

	// force ascending order of freq. cutoff values
	double *freq = (double*)malloc(sizeof(double) * len);

	for(int i = 0; i < len; i++)
		freq[i] = VuoListGetValue_VuoReal(cutoffFrequencies, i+1);

	qsort (freq, len, sizeof(double), compare);

	gam::Biquad<> *low = (*context)->biquad_low;
	gam::Biquad<> *high = (*context)->biquad_high;

	VuoListAppendValue_VuoAudioSamples( (*splitSamples), SamplesWithFilterAndFrequency( samples, low, freq[0]) );

	if(len > 1)
	{
		gam::Biquad<> *mid = (*context)->biquad_band;
		for(int n = 0; n < len-1; n++)
		{
			double lowCutoff = freq[n];
			double highCutoff = freq[n+1];

			VuoListAppendValue_VuoAudioSamples( (*splitSamples), SamplesWithFilterAndFrequency( samples, mid, (highCutoff+lowCutoff)/2.) );
		}
	}

	VuoListAppendValue_VuoAudioSamples( (*splitSamples), SamplesWithFilterAndFrequency( samples, high, freq[len-1]) );

	free(freq);
}

extern "C" void nodeInstanceFini(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	delete (*context)->sync;
	delete (*context)->biquad_low;
	delete (*context)->biquad_band;
	delete (*context)->biquad_high;
}
