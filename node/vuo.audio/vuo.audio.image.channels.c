/**
 * @file
 * vuo.audio.image.channels node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoAudioSamples.h"
#include "VuoList_VuoAudioSamples.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Image from Channels",
					 "keywords" : [ "waveform", "amplitudes" ],
					 "version" : "1.1.1",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoAudioSamples) channels,
		VuoOutputData(VuoImage) image
)
{
	VuoInteger rows = VuoListGetCount_VuoAudioSamples(channels);
	if (!rows)
	{
		*image = NULL;
		return;
	}

	VuoInteger columns = VuoListGetValue_VuoAudioSamples(channels, 1).sampleCount;

	float *pixels = (float *)malloc(rows*columns*sizeof(float));
	VuoAudioSamples *rowSamples = VuoListGetData_VuoAudioSamples(channels);
	for (VuoInteger row = 0; row < rows; ++row)
	{
		for (VuoInteger column = 0; column < MIN(columns, rowSamples[row].sampleCount); ++column)
			pixels[(rows-row-1)*columns + column] = .5 + rowSamples[row].samples[column]*.5;
	}

	*image = VuoImage_makeFromBuffer(pixels, GL_LUMINANCE, columns, rows, VuoImageColorDepth_16, ^(void *buffer){ free(buffer); });
}
