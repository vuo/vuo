/**
 *@file
 * vuo.math.wave.1d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoWave.h"
#include "math.h"

VuoModuleMetadata({
					 "title" : "Make Wave",
					 "description" :
						"<p>Calculates the height of a waveform at a given time.</p> \
						<p>This node can be used to animate an object in a wave motion \
						by sending gradually increasing values to the `time` port. \
						If `waveform` or `period` is changed while the waveform is being animated, \
						the waveform changes smoothly to its new shape.</p> \
						<p><ul> \
						<li>`time` — The time at which to calculate the height.</li> \
						<li>`wave` — The shape of the waveform (sine, triangle, or sawtooth). For more information, see <a href=\"http://en.wikipedia.org/wiki/Waveform\">http://en.wikipedia.org/wiki/Waveform</a>.</li> \
						<li>`period` — The amount of time it takes for the wave to complete a cycle. The waveform's height is the same at `time`, `time + period`, `time + 2*period`, etc.</li> \
						<li>`value` — The calculated height of the waveform, ranging from -1 to 1. At time 0, the height is -1.</li> \
						</ul></p>",
					 "keywords" : [ "sine", "cosine", "sawtooth", "triangle", "phase accumulator", "oscillator", "frequency", "period", "LFO", "VCO", "DCO", "NCO" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


struct phaseAccumulator {
	VuoReal phase;
	VuoReal priorTime;
	VuoWave priorWave;
};

struct phaseAccumulator *nodeInstanceInit()
{
	struct phaseAccumulator *data = (struct phaseAccumulator *)calloc(1,sizeof(struct phaseAccumulator));
	VuoRegister(data, free);
	data->phase = 0;
	data->priorTime = 0;
	data->priorWave = -1;
	return data;
}

void nodeInstanceEvent
(
	VuoInputData(VuoReal, {"default":0}) time,
	VuoInputData(VuoWave, {"default":"sine"}) wave,
	VuoInputEvent(VuoPortEventBlocking_Wall, wave) waveEvent,
	VuoInputData(VuoReal, {"default":1, "suggestedMin":0.000001}) period,
	VuoInputEvent(VuoPortEventBlocking_Wall, period) periodEvent,
	VuoOutputData(VuoReal) value,
	VuoInstanceData(struct phaseAccumulator *) data
)
{
	double positivePeriod = period;
	if (positivePeriod < 0.000001)
		positivePeriod = 0.000001;

	(*data)->phase += (time - (*data)->priorTime) / positivePeriod;
	(*data)->phase = fmod((*data)->phase, 1);
	(*data)->priorTime = time;

	// When changing waveforms, adjust the accumulated phase to make the output value continuous.
	if ((*data)->priorWave != -1 && (*data)->priorWave != wave)
	{
		if ((*data)->priorWave == VuoWave_Sine && wave == VuoWave_Triangle)
		{
			// Solve $-cos(2øπ) = [ 4p-1, -4p+3 ]$ for $p$.
			if ((*data)->phase < 0.5)
				(*data)->phase = (-cos((*data)->phase * 2. * M_PI) + 1.) / 4.;
			else
				(*data)->phase = (-cos((*data)->phase * 2. * M_PI) - 3.) / -4.;
		}
		else if ((*data)->priorWave == VuoWave_Triangle && wave == VuoWave_Sine)
		{
			// Solve $-cos(2øπ) = [ 4p-1, -4p+3 ]$ for $ø$.
			if ((*data)->phase < 0.5)
				(*data)->phase = acos((*data)->phase * -4. + 1.) / (2. * M_PI);
			else
				(*data)->phase = 1. - acos((*data)->phase * 4. - 3.) / (2. * M_PI);
		}
		else if ((*data)->priorWave == VuoWave_Sine && wave == VuoWave_Sawtooth)
			// Solve $-cos(2øπ) = 2p-1$ for $p$.
			(*data)->phase = (-cos((*data)->phase * 2. * M_PI) + 1.) / 2.;
		else if ((*data)->priorWave == VuoWave_Sawtooth && wave == VuoWave_Sine)
			// Solve $-cos(2øπ) = 2p-1$ for $ø$ (always end up on first half of sine phase, to match upward slope of sawtooth).
			(*data)->phase = acos((*data)->phase * -2. + 1.) / (2. * M_PI);
		else if ((*data)->priorWave == VuoWave_Triangle && wave == VuoWave_Sawtooth)
		{
			// Solve $[ 4ø-1, -4ø+3 ] = 2p-1$ for $p$.
			if ((*data)->phase < 0.5)
				(*data)->phase = (*data)->phase * 2.;
			else
				(*data)->phase = (*data)->phase * -2. + 2.;
		}
		else if ((*data)->priorWave == VuoWave_Sawtooth && wave == VuoWave_Triangle)
			// Solve $4ø-1 = 2p-1$ for $ø$ (always end up on first half of triangle phase, to match upward slope of sawtooth).
			(*data)->phase = (*data)->phase / 2.;
	}
	(*data)->priorWave = wave;

	// Calculate the output value.
	if (wave == VuoWave_Sine)
		*value = -cos((*data)->phase * 2. * M_PI);
	else if (wave == VuoWave_Triangle)
	{
		if ((*data)->phase < 0.5)
			*value = (*data)->phase * 4. - 1.;
		else
			*value = (*data)->phase * -4. + 3.;
	}
	else if (wave == VuoWave_Sawtooth)
		*value = (*data)->phase * 2. - 1.;
}

void nodeInstanceFini(
VuoInstanceData(struct phaseAccumulator *) data
) {
}
