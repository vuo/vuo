/**
 * @file
 * vuo.audio.image.waveform node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoAudioSamples.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Waveform Image",
					  "keywords" : [ "amplitudes", "oscilloscope" ],
					  "version" : "2.0.0",
					  "dependencies" : [
						  "VuoCurve"
					  ],
					  "node": {
						  "exampleCompositions" : [ "ShowLiveAudioWaveform.vuo", "ShowStabilizedAudioWaveform.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	VuoAudioSamples priorSamples;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *instance = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->priorSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);
	bzero(instance->priorSamples.samples, VuoAudioSamples_bufferSize * sizeof(VuoReal));
	VuoAudioSamples_retain(instance->priorSamples);

	return instance;
}

static VuoReal vuo_audio_image_waveform_getSample(const VuoAudioSamples *prior, const VuoAudioSamples *current, VuoInteger sampleIndex)
{
	if (sampleIndex < prior->sampleCount)
		return prior->samples[MAX(0, sampleIndex)];
	else
		return current->samples[MIN(current->sampleCount - 1, sampleIndex - prior->sampleCount)];
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoAudioSamples) samples,
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":0.25}}) fillColor,
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) lineColor,
		VuoInputData(VuoInteger, {"default":256, "suggestedMin":3, "suggestedMax":1024}) height,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-0.9, "suggestedMax":0.9, "suggestedStep":0.1}) syncAmplitude,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":-1.0, "suggestedMax":1.0, "suggestedStep":0.1}) syncCenter,
		VuoInputData(VuoBoolean, {"default":false}) attenuateEnds,
		VuoOutputData(VuoImage) image
)
{
	if (!samples.samples)
		return;

	VuoInteger pixelsWide = MIN((*instance)->priorSamples.sampleCount, samples.sampleCount);
	VuoInteger pixelsHigh = MAX(3, height);
	VuoInteger positiveAmplitude = (pixelsHigh-1)/2;
	unsigned char *pixels = (unsigned char *)calloc(1, pixelsWide*pixelsHigh*4);

	VuoInteger sampleIndex;

	// Start back at the trigger crossing.
	{
		VuoInteger centerOffset = (VuoReal_clamp(syncCenter, -1, 1)/2. + .5) * VuoAudioSamples_bufferSize;

		for (sampleIndex = 1 + centerOffset; sampleIndex < VuoAudioSamples_bufferSize + centerOffset; ++sampleIndex)
			if (vuo_audio_image_waveform_getSample(&(*instance)->priorSamples, &samples, sampleIndex - 1) <  syncAmplitude
			 && vuo_audio_image_waveform_getSample(&(*instance)->priorSamples, &samples, sampleIndex    ) >= syncAmplitude)
				break;

		sampleIndex -= centerOffset;
	}

	for (VuoInteger column = 0; column < pixelsWide; ++column, ++sampleIndex)
	{
		double position = (double)column / pixelsWide;
		const double attenuationPower = 2.;
		double attenuation = attenuateEnds
			? 1. - pow(1. - (1. - cos(position * 2 * M_PI)) / 2., attenuationPower)
			: 1.;

		VuoReal previousSample = vuo_audio_image_waveform_getSample(&(*instance)->priorSamples, &samples, sampleIndex - 1) * attenuation;
		VuoReal currentSample  = vuo_audio_image_waveform_getSample(&(*instance)->priorSamples, &samples, sampleIndex    ) * attenuation;
		VuoReal nextSample     = vuo_audio_image_waveform_getSample(&(*instance)->priorSamples, &samples, sampleIndex + 1) * attenuation;

		VuoReal minSample = MIN(MIN(previousSample,currentSample),nextSample);
		VuoReal maxSample = MAX(MAX(previousSample,currentSample),nextSample);

		// Plot a vertical line that spans from the center to the outer sample.
		{
			VuoReal outerSample = currentSample > 0 ? maxSample : minSample;

			VuoInteger startRow			= positiveAmplitude;
			VuoInteger endRowCurrent	= MIN(MAX(positiveAmplitude * (1. + currentSample), 0), pixelsHigh-1);
			VuoInteger endRowOuter		= MIN(MAX(positiveAmplitude * (1. + outerSample), 0), pixelsHigh-1);

			if (startRow < endRowCurrent)
				for (VuoInteger row = startRow; row <= endRowOuter; ++row)
				{
					VuoReal intensity = VuoReal_curve(row-endRowCurrent+1, 255.*fillColor.a, 0, endRowOuter-endRowCurrent+1, VuoCurve_Linear, VuoCurveEasing_InOut, VuoLoopType_None);
					pixels[(row*pixelsWide + column)*4 + 2] = intensity*fillColor.r;
					pixels[(row*pixelsWide + column)*4 + 1] = intensity*fillColor.g;
					pixels[(row*pixelsWide + column)*4 + 0] = intensity*fillColor.b;
					pixels[(row*pixelsWide + column)*4 + 3] = intensity;
				}
			else
				for (VuoInteger row = startRow; row >= endRowOuter; --row)
				{
					VuoReal intensity = VuoReal_curve(row-endRowCurrent-1, 255.*fillColor.a, 0, endRowOuter-endRowCurrent-1, VuoCurve_Linear, VuoCurveEasing_InOut, VuoLoopType_None);
					pixels[(row*pixelsWide + column)*4 + 2] = intensity*fillColor.r;
					pixels[(row*pixelsWide + column)*4 + 1] = intensity*fillColor.g;
					pixels[(row*pixelsWide + column)*4 + 0] = intensity*fillColor.b;
					pixels[(row*pixelsWide + column)*4 + 3] = intensity;
				}
		}

		// Plot a vertical line that spans the previous sample, the current sample, and the next sample.
		{
			VuoInteger startRow	= MIN(MAX(positiveAmplitude * (1. + minSample), 0), pixelsHigh-1);
			VuoInteger endRow	= MIN(MAX(positiveAmplitude * (1. + maxSample), 0), pixelsHigh-1);

			for (VuoInteger row = startRow; row <= endRow; ++row)
			{
				VuoReal intensity = VuoReal_curve(row-startRow+1, 0, 255.*lineColor.a, (endRow-startRow)/2. + 1, VuoCurve_Linear, VuoCurveEasing_InOut, VuoLoopType_Mirror);
				pixels[(row*pixelsWide + column)*4 + 2] = MIN(pixels[(row*pixelsWide + column)*4 + 2] + intensity*lineColor.r, 255);
				pixels[(row*pixelsWide + column)*4 + 1] = MIN(pixels[(row*pixelsWide + column)*4 + 1] + intensity*lineColor.g, 255);
				pixels[(row*pixelsWide + column)*4 + 0] = MIN(pixels[(row*pixelsWide + column)*4 + 0] + intensity*lineColor.b, 255);
				pixels[(row*pixelsWide + column)*4 + 3] = MIN(pixels[(row*pixelsWide + column)*4 + 3] + intensity, 255);
			}
		}
	}

	*image = VuoImage_makeFromBuffer(pixels, GL_BGRA, pixelsWide, pixelsHigh, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });

	VuoAudioSamples_release((*instance)->priorSamples);
	(*instance)->priorSamples = samples;
	VuoAudioSamples_retain((*instance)->priorSamples);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoAudioSamples_release((*instance)->priorSamples);
}
