/**
 * @file
 * vuo.audio.analyze.loudness node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					 "title" : "Calculate Loudness",
					 "keywords" : [ "sound", "music", "volume", "gain", "rms", "root mean square", "decibel" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoAudio"
					 ],
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "VisualizeLoudness.vuo" ],
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoAudioSamples) audioSamples,
		VuoOutputData(VuoReal) loudness
)
{
	if (audioSamples.sampleCount == 0)
	{
		*loudness = 0;
		return;
	}

	float rms = 0;
	
	for(int j = 0; j < audioSamples.sampleCount; j++)
		rms += pow(audioSamples.samples[j], 2);
	rms /= audioSamples.sampleCount;

	*loudness = pow( pow(rms, .5), 1/4.);
}
