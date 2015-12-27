/**
 * @file
 * vuo.audio.multiply node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
                     "title" : "Multiply Audio Channels",
                     "keywords" : [ "sound", "music", "merge", "combine", "AM Synthesis", "multiply" ],
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
  
  (*multipliedSamples).samplesPerSecond = VuoAudioSamples_sampleRate;

  for(unsigned int n = 0; n < (*multipliedSamples).sampleCount; n++) //clear output buffer
       (*multipliedSamples).samples[n] = 0.;
  
  int firstAudioPort = -1;  
  
  for(unsigned int i = 0; i < channelCount; i++)
  {
    VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(samples, i+1);
    
    if(VuoAudioSamples_isPopulated(as) && firstAudioPort == -1){ //check if we have audio samples & if this is first time loop has run
       firstAudioPort = i;  // get number of first audio port
    }
if(VuoAudioSamples_isPopulated(as)){  


  if(i == firstAudioPort){ // we need to load the first audio channel into the output buffer - otherwise the output buffer will be full of 0's when we do *= next
      for(unsigned int n = 0; n < as.sampleCount; n++) 
      (*multipliedSamples).samples[n] = as.samples[n]; // loading the 1st audio channel into output buffer
    }
    else{
    for(unsigned int n = 0; n < as.sampleCount; n++)
    (*multipliedSamples).samples[n] *= as.samples[n];
    }
}
  }
}
