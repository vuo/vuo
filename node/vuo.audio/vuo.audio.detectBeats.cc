/**
 * @file
 * vuo.audio.detectBeats node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "node.h"
#include "VuoDsp.h"
#include "VuoWave.h"
#include "VuoAudioSamples.h"
#include "VuoTempoRange.h"

VuoModuleMetadata({
					  "title" : "Detect Audio Beats",
					  "keywords" : [ "tempo", "BPM", "synchronize", "metronome", "track" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoBeatDetektor",
						  "VuoDsp"
					  ],
					  "node": {
						  "exampleCompositions" : [ "BounceDotsToTheBeat.vuo", "JumpDotToTheBeat.vuo" ],
					  }
				 });
}

#include "VuoBeatDetektor.hh"

struct nodeInstanceData
{
	VuoReal *sampleQueue;
	VuoDsp vdsp;

	BeatDetektor *detektor;
	int priorSixteenthCount;
	int beatCount;
	int offset;
	VuoReal beatTime;
	VuoReal priorClock;
	VuoTempoRange priorTempoRange;
};

const int binSize = 4096;
const int finishLine = 200;

extern "C" struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sampleQueue = (VuoReal *)calloc(binSize, sizeof(VuoReal));

	context->vdsp = VuoDsp_make(binSize, VuoWindowing_Blackman);
	VuoRetain(context->vdsp);

	context->priorTempoRange = VuoTempoRange_Moderato;
	int bpmMin = VuoTempoRange_getBaseBPM(context->priorTempoRange);
	context->detektor = new BeatDetektor(bpmMin, bpmMin*2-1);
	context->detektor->finish_line = finishLine;


	return context;
}

extern "C" void nodeInstanceEvent
(
		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputData(VuoAudioSamples) samples,
		VuoInputData(VuoTempoRange, {"default":"moderato"}) tempoRange,
		VuoInputEvent({"eventBlocking":"wall","data":"samples"}) samplesEvent,
		VuoInputEvent({"eventBlocking":"wall"}) nudge,
		VuoInputEvent({"eventBlocking":"wall"}) reset,
		VuoOutputData(VuoReal, {"name":"Beats per Minute"}) beatsPerMinute,
		VuoOutputEvent({"data":"beatsPerMinute"}) beatsPerMinuteEvent,
		VuoOutputEvent() beat,
		VuoOutputData(VuoReal) clock,
		VuoOutputEvent({"data":"clock"}) clockEvent,
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if (tempoRange != (*context)->priorTempoRange)
	{
		delete (*context)->detektor;
		(*context)->priorTempoRange = tempoRange;
		int bpmMin = VuoTempoRange_getBaseBPM(tempoRange);
		(*context)->detektor = new BeatDetektor(bpmMin, bpmMin*2-1);
		(*context)->detektor->finish_line = finishLine;
	}

	if (!samples.samples || !samples.sampleCount)
		return;

	if (reset)
	{
		(*context)->detektor->reset();
		(*context)->beatCount = 0;
		(*context)->offset = 0;
		(*context)->priorClock = 0;
		(*context)->priorSixteenthCount = 0;
	}

	if (nudge)
		(*context)->offset = ((*context)->offset + 1) % 4;

	if (samplesEvent)
	{
		// Build a ringbuffer for the FFT.
		unsigned long sampleCount = (unsigned long)samples.sampleCount;
		memmove((*context)->sampleQueue, (*context)->sampleQueue + sampleCount, (binSize - sampleCount) * sizeof(VuoReal));
		memcpy((*context)->sampleQueue + (binSize - sampleCount), samples.samples, sampleCount * sizeof(VuoReal));

		// Process the FFT.
		unsigned int amplitudesCount;
		VuoReal *amplitudesArray = VuoDsp_frequenciesForSamples((*context)->vdsp, (*context)->sampleQueue, binSize, VuoAudioBinAverageType_None, &amplitudesCount);

		// Convert to vector<float> since that's what BeatDetektor expects.
		std::vector<float> amplitudesFloats(amplitudesCount);
		float *amplitudesFloatsArray = &(amplitudesFloats[0]);
		for (VuoInteger i = 0; i < amplitudesCount; ++i)
			amplitudesFloatsArray[i] = (float)amplitudesArray[i];

		// Execute the BeatDetektor.
		VuoReal t = VuoLogGetElapsedTime();
		(*context)->detektor->process((float)t, amplitudesFloats);
	}

	if (timeEvent && (*context)->detektor->quality_avg > finishLine && (*context)->detektor->win_bpm_int)
	{
		*beatsPerMinute = (float)(*context)->detektor->win_bpm_int/10.;
		*beatsPerMinuteEvent = true;

		*beat = false;
		if ((*context)->priorSixteenthCount != 0)
			if ((*context)->detektor->quarter_counter != (*context)->priorSixteenthCount)
				if (((*context)->detektor->quarter_counter + (*context)->offset) % 4 == 0)
				{
					*beat = true;
					(*context)->beatTime = time;
					++(*context)->beatCount;
				}
		(*context)->priorSixteenthCount = (*context)->detektor->quarter_counter;

		VuoReal proposedClock = (*context)->beatCount + (time - (*context)->beatTime) * *beatsPerMinute/60 - 1;
		if ((*context)->beatCount && proposedClock >= (*context)->priorClock)
		{
			*clock = proposedClock;
			*clockEvent = true;
		}

//		VLog("%3.1f bpm  q=%f  %15s  %s",
//			 (float)(*context)->detektor->win_bpm_int/10.,
//			 (*context)->detektor->quality_avg,
//			 *clockEvent ? VuoText_format("clock=%f",*clock) : "",
//			 *beat ? "*" : ""
//		);
	}
}

extern "C" void nodeInstanceFini(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	delete (*context)->detektor;
	VuoRelease((*context)->vdsp);
	free((*context)->sampleQueue);
}
