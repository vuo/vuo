Creates an audio sample buffer from a list of real number values.

In Vuo, audio sample buffers are always a fixed size of 512 samples.  If the supplied list size is greater than 512, the list will be truncated.  If the list contains fewer than 512 samples, the output sample buffer will be filled with 0 to meet the required size.
