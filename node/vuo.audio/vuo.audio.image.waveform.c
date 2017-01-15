/**
 * @file
 * vuo.audio.image.waveform node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoAudioSamples.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make Waveform Image",
					  "keywords" : [ "amplitudes", "oscilloscope" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoCurve"
					  ],
					  "node": {
						  "exampleCompositions" : [ "ShowLiveAudioWaveform.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoAudioSamples) samples,
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":0.25}}) fillColor,
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) lineColor,
		VuoInputData(VuoInteger, {"default":128, "suggestedMin":1, "suggestedMax":1024}) amplitude,
		VuoOutputData(VuoImage) image
)
{
	if (!samples.samples)
		return;

	VuoInteger positiveAmplitude = MAX(fabs(amplitude),1);

	VuoInteger pixelsWide = samples.sampleCount;
	VuoInteger pixelsHigh = positiveAmplitude*2 + 1;
	unsigned char *pixels = (unsigned char *)calloc(1, pixelsWide*pixelsHigh*4);

	for (VuoInteger column = 0; column < pixelsWide; ++column)
	{
		VuoReal previousSample	= samples.samples[MAX(0, column-1)];
		VuoReal currentSample	= samples.samples[column];
		VuoReal nextSample		= samples.samples[MIN(pixelsWide-1, column+1)];

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
}
