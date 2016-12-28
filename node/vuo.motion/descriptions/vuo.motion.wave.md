Calculates the height of a [waveform](http://en.wikipedia.org/wiki/Waveform) at a given time.

This node can be used to animate an object in a wave motion by sending gradually increasing values to the `Time` port. 
If `Wave` or `Period` is changed while the waveform is being animated, the waveform changes smoothly to its new shape.

   - `Time` — The time at which to calculate the waveform height.
   - `Wave` — The shape of the waveform (sine, triangle, or sawtooth).
   - `Period` — The amount of time it takes for the wave to complete a cycle. The wave's height is the same at `Time`, `Time + Period`, `Time + 2*Period`, and so on (except while transitioning to a different waveform shape after a change to `Wave` or `Period`).
   - `Center` — The starting point for the wave.  When time is at half of the period this will be the output value.
   - `Amplitude` — The range of the wave value between start and halfway to `Period`.  An `Amplitude` of 1 means that the waveform will range from -1 to 1 (assuming that `Center` is 0).
   - `Phase` — The offset into the wave cycle.  At 1, the phase is back to the beginning of the cycle.  Useful if you have multiple `Wave` nodes and want them to move slightly out of sync.
   - `Value` — The calculated height of the waveform, ranging from -1 to 1. At time 0, the height is -1.
