Calculates the height of a [waveform](http://en.wikipedia.org/wiki/Waveform) at a given time.

This node can be used to animate an object in a wave motion by sending gradually increasing values to the `time` port. 
If `wave` or `period` is changed while the waveform is being animated, the waveform changes smoothly to its new shape.

   - `time` — The time at which to calculate the waveform height.
   - `wave` — The shape of the waveform (sine, triangle, or sawtooth).
   - `period` — The amount of time it takes for the wave to complete a cycle. The wave's height is the same at `time`, `time + period`, `time + 2*period`, and so on (except while transitioning to a different waveform shape after a change to `wave` or `period`).
   - `center` - The starting point for the wave.  When time is at half of the period this will be the output value.
   - `amplitude` - The range of the wave value between start and halfway to `period`.  An `amplitude` of 1 means that the waveform will range from -1 to 1 (assuming that `center` is 0).
   - `value` — The calculated height of the waveform, ranging from -1 to 1. At time 0, the height is -1.
