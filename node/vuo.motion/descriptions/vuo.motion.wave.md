Calculates the height of a [waveform](http://en.wikipedia.org/wiki/Waveform) at a given time.

This node can be used to animate an object in a wave motion by sending gradually increasing values to the `Time` port. 
If `Wave` or `Period` is changed while the waveform is being animated, the waveform changes smoothly to its new shape.

   - `Time` — The time at which to calculate the waveform height.
   - `Wave` — The shape of the waveform (sine, triangle, or sawtooth).
   - `Period` — The amount of time it takes for the wave to complete a cycle. The wave's height is the same at `Time`, `Time + Period`, `Time + 2*Period`, and so on (except while transitioning to a different waveform shape after a change to `Wave` or `Period`).
   - `Center` — The vertical midpoint of the wave.
   - `Amplitude` — The range of the wave value between start and halfway to `Period`.  An `Amplitude` of 1 means that the waveform will range from -1 to 1 (assuming that `Center` is 0).
   - `Phase` — The offset into the wave cycle at `Time` = 0. A phase of 0.5 means that the wave is shifted by half of a cycle. A phase of 1 means that the wave is shifted by a full cycle, which is the same as a phase of 0. Useful if you have multiple `Wave` nodes and want them to move slightly out of sync.
   - `Value` — The calculated height of the waveform, ranging from `Center - Amplitude` to `Center + Amplitude`. When `Time` and `Phase` are both 0, the height is `Center - Amplitude`.
