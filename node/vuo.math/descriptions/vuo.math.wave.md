Calculates the height of a waveform at a given time.

This node can be used to animate an object in a wave motion by sending gradually increasing values to the `time` port. 
If `waveform` or `period` is changed while the waveform is being animated, the waveform changes smoothly to its new shape.

   - `time` — The time at which to calculate the height.
   - `wave` — The shape of the waveform (sine, triangle, or sawtooth). For more information, see [Waveform on Wikipedia](http://en.wikipedia.org/wiki/Waveform). 
   - `period` — The amount of time it takes for the wave to complete a cycle. The waveform's height is the same at `time`, `time + period`, `time + 2*period`, etc.
   - `value` — The calculated height of the waveform, ranging from -1 to 1. At time 0, the height is -1.
