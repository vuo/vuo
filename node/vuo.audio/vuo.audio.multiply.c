#include "node.h"

VuoModuleMetadata({
                     "title" : "Multiply 2 Audio Channels",
                     "keywords" : [ "sound", "music", "merge", "combine", "FM Synthesis" ],
                     "version" : "0.0.1",
                     "node": {
                       //  "exampleCompositions" : [ "PanAudio.vuo" ],
                     }
                 });

void nodeEvent
(
        VuoInputData(VuoList_VuoAudioSamples) samples,
        VuoOutputData(VuoAudioSamples) mixedSamples
)
{
  unsigned int channelCount = VuoListGetCount_VuoAudioSamples(samples);

  *mixedSamples = VuoAudioSamples_alloc(VuoAudioSamples_bufferSize);

  for(unsigned int n = 0; n < (*mixedSamples).sampleCount; n++)
  
  (*mixedSamples).samples[n] = 0.;

  for(unsigned int i = 0; i < channelCount; i++)
  {
    VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(samples, i+1);

  if(i == 0)
    {
      for(unsigned int n = 0; n < as.sampleCount; n++)
      (*mixedSamples).samples[n] = as.samples[n];
    }
    for(unsigned int n = 0; n < as.sampleCount; n++)
    (*mixedSamples).samples[n] *= as.samples[n];
  }
}
