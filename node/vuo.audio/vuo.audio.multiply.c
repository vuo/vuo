/**
 * @file
 * vuo.audio.mix node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
                     "title" : "Multiply Audio Channels",
                     "keywords" : [ "sound", "music", "merge", "combine", "FM Synthesis", "multiply" ],
                     "version" : "0.0.1",
                     "node": {
                        "exampleCompositions" : [ ]
                     }
                 });

void nodeEvent
(
        VuoInputData(VuoList_VuoAudioSamples) samples,
        VuoOutputData(VuoAudioSamples) multipliedSamples
)
{
  unsigned int channelCount = VuoListGetCount_VuoAudioSamples(samples);

  *multipliedSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);

  for(unsigned int n = 0; n < (*multipliedSamples).sampleCount; n++)
  
  (*multipliedSamples).samples[n] = 0.;

  for(unsigned int i = 0; i < channelCount; i++)
  {
    VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(samples, i+1);

  if(i == 0)
    {
      for(unsigned int n = 0; n < as.sampleCount; n++)
      (*multipliedSamples).samples[n] = as.samples[n];
    }
    for(unsigned int n = 0; n < as.sampleCount; n++)
    (*multipliedSamples).samples[n] *= as.samples[n];
  }
}
