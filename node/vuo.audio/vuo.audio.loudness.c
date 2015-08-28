/**
 * @file
 * vuo.audio.loudness node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					 "title" : "Adjust Loudness",
					 "keywords" : [ "sound", "music", "gain", "decibel", "volume" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoAudio"
					 ],
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "ControlLoudness.vuo", "PanAudio.vuo" ],
					 }
				 });

struct nodeInstanceData
{
	VuoReal loudness;
};

struct nodeInstanceData *nodeInstanceInit
(
		VuoInputData(VuoReal) loudness
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->loudness = loudness;

	return context;
}

void nodeInstanceEvent
(
		VuoInputData(VuoList_VuoAudioSamples) samples,
		VuoInputData(VuoReal, {"suggestedMin":0, "suggestedMax":2, "default":1}) loudness,
		VuoOutputData(VuoList_VuoAudioSamples) adjustedSamples,
		VuoInstanceData(struct nodeInstanceData *)context
)
{
	unsigned int len = VuoListGetCount_VuoAudioSamples(samples);
	*adjustedSamples = VuoListCreate_VuoAudioSamples();

	VuoReal clampedLoudness = MAX(0, loudness);

	/**
	 *	could collapse to a single loop, but then it would have to lerp
	 *  every time, no matter what.  so here's an if() that creates some
	 *  duplicate code but saves lerps.  ...lerp.
	 */
	if (clampedLoudness != (*context)->loudness)
	{
		float v;
		float oldLoudness = (*context)->loudness;

		for(int i = 0; i < len; i++)
		{
			VuoAudioSamples as = VuoListGetValueAtIndex_VuoAudioSamples(samples, i+1);
			VuoAudioSamples nas = VuoAudioSamples_alloc(as.sampleCount);
			nas.samplesPerSecond = as.samplesPerSecond;
			unsigned int sampleCount = as.sampleCount;

			for(int n = 0; n < sampleCount; n++)
			{
				v = VuoReal_lerp(oldLoudness, clampedLoudness, (float)n / sampleCount);
				// Power of 4 provides dynamic range of ~60 dB
				// http://www.dr-lex.be/info-stuff/volumecontrols.html
				nas.samples[n] = as.samples[n] * pow(v, 4);
				// Add a linear taper to absolute silence.
				if (v < 0.1)
					nas.samples[n] *= v*10;
			}

			VuoListAppendValue_VuoAudioSamples(*adjustedSamples, nas);
		}

		(*context)->loudness = clampedLoudness;
	}
	else
	{
		for(int i = 0; i < len; i++)
		{
			VuoAudioSamples as = VuoListGetValueAtIndex_VuoAudioSamples(samples, i+1);
			VuoAudioSamples nas = VuoAudioSamples_alloc(as.sampleCount);
			nas.samplesPerSecond = as.samplesPerSecond;

			for(int n = 0; n < as.sampleCount; n++)
			{
				// Power of 4 provides dynamic range of ~60 dB
				// http://www.dr-lex.be/info-stuff/volumecontrols.html
				nas.samples[n] = as.samples[n] * pow(clampedLoudness, 4);
				// Add a linear taper to absolute silence.
				if (clampedLoudness < 0.1)
					nas.samples[n] *= clampedLoudness*10;
			}

			VuoListAppendValue_VuoAudioSamples(*adjustedSamples, nas);
		}
	}
}

void nodeInstanceFini(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
