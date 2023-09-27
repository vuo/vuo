/**
 * @file
 * vuo.audio.image.channels node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioSamples.h"
#include "VuoList_VuoAudioSamples.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Image from Channels",
					 "keywords" : [ "waveform", "amplitudes" ],
					 "version" : "1.2.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static double passthru(double a)
{
	return a;
}

void nodeEvent
(
		VuoInputData(VuoList_VuoAudioSamples) channels,
		VuoInputData(VuoDiode, {"default":"bipolar"}) range,
		VuoOutputData(VuoImage) image
)
{
	VuoInteger rows = VuoListGetCount_VuoAudioSamples(channels);
	if (!rows)
	{
		*image = NULL;
		return;
	}

	typedef double (*funcType)(double);
	funcType func = passthru;
	double m, b = 0;
	if (range == VuoDiode_Unipolar)
		m = b = .5;
	else if (range == VuoDiode_Bipolar)
		m = 1;
	else // if (range == VuoDiode_Absolute)
	{
		m = 1;
		func = fabs;
	}

	VuoInteger columns = VuoListGetValue_VuoAudioSamples(channels, 1).sampleCount;

	float *pixels = (float *)malloc(rows*columns*sizeof(float));
	VuoAudioSamples *rowSamples = VuoListGetData_VuoAudioSamples(channels);
	for (VuoInteger row = 0; row < rows; ++row)
	{
		for (VuoInteger column = 0; column < MIN(columns, rowSamples[row].sampleCount); ++column)
			pixels[(rows-row-1)*columns + column] = func(m * rowSamples[row].samples[column] + b);
	}

	*image = VuoImage_makeFromBuffer(pixels, GL_LUMINANCE, columns, rows, VuoImageColorDepth_32, ^(void *buffer){ free(buffer); });
}
