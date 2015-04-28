/**
 * @file
 * vuo.audio.image.channels node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
					 "version" : "1.0.0"
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

	VuoInteger columns = VuoListGetValueAtIndex_VuoAudioSamples(channels, 1).sampleCount;

	unsigned char *pixels = (unsigned char *)calloc(1, rows*columns);
	for (VuoInteger row = 0; row < rows; ++row)
	{
		VuoAudioSamples rowSamples = VuoListGetValueAtIndex_VuoAudioSamples(channels, row+1);
		for (VuoInteger column = 0; column < MIN(columns, rowSamples.sampleCount); ++column)
			pixels[row*columns + column] = 127.5 + rowSamples.samples[column]*127.5;
	}

	*image = VuoImage_makeFromBuffer(pixels, GL_LUMINANCE, columns, rows);

	free(pixels);
}
