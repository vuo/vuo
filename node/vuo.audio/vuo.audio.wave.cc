/**
 * @file
 * vuo.audio.wave node implementation.
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
					  "title" : "Make Audio Wave",
					  "keywords" : [ "sine", "cosine", "sawtooth", "triangle", "phase accumulator", "oscillator", "frequency", "period", "LFO", "VCO", "DCO", "NCO" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "Gamma"
					  ],
					  "node": {
						  "exampleCompositions" : [ "PlayAudioWave.vuo", "PlayBluesOrgan.vuo", "PanAudio.vuo", "SynchronizeOscillators.vuo" ],
					  }
				 });
}

#include "Gamma/Oscillator.h"
#include "Gamma/Sync.h"

struct nodeInstanceData {
	gam::Sync *sync;
	gam::Sine<> *sine;
	gam::Saw<> *saw;
};

extern "C" struct nodeInstanceData *nodeInstanceInit
(
//		VuoInputData(VuoReal, {"default":48000.0, "suggestedMin":0.000001, "suggestedStep":100.0}) sampleRate,
		VuoInputData(VuoWave, {"default":"sine"}) wave,
		VuoInputData(VuoReal, {"default":440.0, "suggestedMin":0.000001}) frequency,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) setPhase
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->sync = new gam::Sync(VuoAudioSamples_sampleRate);

	context->sine = new gam::Sine<>;
	*(context->sync) << *(context->sine);
	context->sine->freq(frequency);
	context->sine->phase(setPhase);

	context->saw = new gam::Saw<>;
	*(context->sync) << *(context->saw);
	context->saw->freq(frequency);
	context->saw->phase(setPhase);

	return context;
}

extern "C" void nodeInstanceEvent
(
//		VuoInputData(VuoReal, {"default":48000.0, "suggestedMin":0.000001, "suggestedStep":100.0}) sampleRate,
//		VuoInputEvent({"eventBlocking":"wall","data":"sampleRate"}) sampleRateEvent,
		VuoInputData(VuoWave, {"default":"sine"}) wave,
		VuoInputEvent({"eventBlocking":"wall","data":"wave"}) waveEvent,
		VuoInputData(VuoReal, {"default":440.0, "suggestedMin":0.000001}) frequency,
		VuoInputEvent({"eventBlocking":"wall","data":"frequency"}) frequencyEvent,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) setPhase,
		VuoInputEvent({"eventBlocking":"wall", "data":"setPhase", "hasPortAction":true}) setPhaseEvent,
		VuoOutputData(VuoAudioSamples) samples,
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	double sampleRate = VuoAudioSamples_sampleRate;
	(*context)->sync->spu(sampleRate);

	(*context)->sine->freq(frequency);
	(*context)->saw->freq(frequency);

	if (setPhaseEvent)
	{
		(*context)->sine->phase(setPhase);
		(*context)->saw->phase(setPhase);
	}

	// Only calculate the output samples (and thus increment the phase) if the event is going to make it to the output port.
	if (waveEvent || frequencyEvent || setPhaseEvent)
		return;

	*samples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
	samples->samplesPerSecond = VuoAudioSamples_sampleRate;

	for (VuoInteger i=0; i<VuoAudioSamples_bufferSize; ++i)
	{
		gam::Sine<> *s = (*context)->sine;
		double sine = (*s)();

		gam::Saw<> *sawOsc = (*context)->saw;
		double saw = (*sawOsc)() * 6.;

		if (wave == VuoWave_Sine)
			samples->samples[i] = sine;
		else if (wave == VuoWave_Sawtooth)
			samples->samples[i] = saw;
		else if (wave == VuoWave_Triangle)
			samples->samples[i] = (gam::scl::triangle(UINT32_MAX * (sawOsc->phase() / (M_PI * 2))) - 0.833) * 6.;
	}
}

extern "C" void nodeInstanceFini(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	delete (*context)->sync;
}
