/**
 * @file
 * vuo.audio.analyze.loudness node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Calculate Loudness",
					 "keywords" : [ "sound", "music", "volume", "gain", "rms", "root mean square", "decibel" ],
					 "version" : "2.0.0",
					 "node" : {
						 "exampleCompositions" : [ "VisualizeLoudness.vuo" ],
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoAudioSamples) samples,
		VuoOutputData(VuoReal) loudness
)
{
	if (samples.sampleCount == 0)
	{
		*loudness = 0;
		return;
	}

	float rms = 0;

	for(int j = 0; j < samples.sampleCount; j++)
		rms += pow(samples.samples[j], 2);
	rms /= samples.sampleCount;

	*loudness = pow( pow(rms, .5), 1/4.);
}
