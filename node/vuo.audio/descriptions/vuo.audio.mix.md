Combines multiple audio channels into a single channel.

- `Samples` - A list of audio sample buffers, each corresponding to one channel.
- `Mixed Samples` - A single audio buffer consisting of all channels added together.

If the input audio samples range between -1 and 1, then the mixed samples may fall outside the range of -1 to 1, resulting in distorted sound if you try to play them on an audio device. To avoid this, scale the loudness of the combined *N* channels of audio by (1/*N*)^(1/4) using the `Adjust Loudness` (`vuo.audio.loudness`) node.
