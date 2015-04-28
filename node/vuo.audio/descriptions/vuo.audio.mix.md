Combines multiple audio channels into a single channel.

- `audioSamples` - A list of audio sample buffers, each corresponding to one channel.
- `mixedSamples` - A single audio buffer consisting of all channels added together.

If the input audio samples range between -1 and 1, then the mixed samples may fall outside the range of -1 to 1, resulting in distorted sound if you try to play them on an audio device. To avoid this, make sure the N channels of input audio samples range between -1/N and 1/N. 
