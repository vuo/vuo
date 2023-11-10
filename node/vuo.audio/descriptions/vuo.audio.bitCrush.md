Produces distortion by quantizing audio samples.

The `Bit Depth` input port specifies, in bits, the resolution of the output sample amplitudes.  The lower the bit depth, the more intense the distortion.  When the bit depth is 1, audio samples will only have 2 possible values, -1 and +1, producing a very harsh sound since there is no room for subtle variation in amplitude.  At 2-bit resolution, there are 4 values: -1, -1/3, +1/3, and +1.  At 3-bit resolution, there are 8 values, and so on.

By default, Vuo processes audio at 64-bit floating point resolution.  Typical computer audio hardware inputs and outputs audio at 16-bit or 24-bit resolution.  Older digital audio hardware typically used lower resolutions — for example, the EMU SP-12 sampler (released in 1985) had 12-bit resolution, the Linn 9000 drum machine used 8-bit samples, and the Roland TR-909 drum machine used 6-bit samples.

Thanks to [alexmitchellmus](https://community.vuo.org/u/alexmitchellmus) for contributing this node.
