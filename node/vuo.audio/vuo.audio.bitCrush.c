/**
 * @file
 * vuo.audio.bitCrush node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <math.h>

VuoModuleMetadata({
					 "title" : "Bit Crush Audio",
					 "keywords" : [
						 "sound", "music", "filter",
						 "distortion", "bits", "quantize", "lo-fi", "lofi", "8-bit", "8bit", "retro",
					 ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "CrushSineWave.vuo" ],
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":16, "suggestedMin":1, "suggestedMax":16}) bitDepth,
		VuoInputData(VuoAudioSamples) samples,
		VuoOutputData(VuoAudioSamples) crushedSamples
)
{
	*crushedSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
	(*crushedSamples).samplesPerSecond = VuoAudioSamples_sampleRate;

	if (!samples.samples)
	{
		for (unsigned int n = 0; n < VuoAudioSamples_bufferSize; n++)
			(*crushedSamples).samples[n] = 0.;
		return;
	}

	double clampedBitDepth = VuoReal_clamp(bitDepth, 1, 64);

	// Match Ableton Live's behavior:
	// When bitDepth = 1, there should be just 2 values: -1 and +1.
	// When bitDepth = 2, there should be 4 values: -1, -1/3, +1/3, and +1.
	VuoReal bitDepthValue = (pow(2, clampedBitDepth) - 1) / 2.;
	for (unsigned int n = 0; n < samples.sampleCount; n++)
		(*crushedSamples).samples[n] = round( (samples.samples[n] + 1) * bitDepthValue ) / bitDepthValue - 1;
}
