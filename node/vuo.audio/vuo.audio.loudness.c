/**
 * @file
 * vuo.audio.loudness node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Adjust Loudness",
					 "keywords" : [ "sound", "music", "gain", "decibel", "volume" ],
					 "version" : "2.0.0",
					 "node": {
						 "exampleCompositions" : [ "ControlLoudness.vuo", "PanAudio.vuo" ],
					 }
				 });

struct nodeInstanceData
{
	VuoReal loudness;
};

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	// Always start out at 0, then lerp to the desired loudless, to avoid clicks/blips upon launch.
	context->loudness = 0;

	return context;
}

void nodeInstanceEvent
(
		VuoInputData(VuoList_VuoAudioSamples) samples,
		VuoInputEvent({"data":"samples"}) samplesEvent,
		VuoInputData(VuoReal, {"suggestedMin":0, "suggestedMax":2, "default":1}) loudness,
		VuoInputEvent({"data":"loudness", "eventBlocking":"wall"}) loudnessEvent,
		VuoOutputData(VuoList_VuoAudioSamples) adjustedSamples,
		VuoInstanceData(struct nodeInstanceData *)context
)
{
	if (!samplesEvent)
		return;

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
			VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(samples, i+1);
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
			VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(samples, i+1);
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
