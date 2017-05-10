Creates a graph of an audio waveform, where the height of the curve is determined by the amplitude of each audio sample.

   - `Fill Color` — The color of the area between the curve and the axis
   - `Line Color` — The color of the curve.
   - `Amplitude` — The curve's maximum height, in pixels.

The output image's width is 512 pixels (the size of a sample buffer), and its height is `2*Amplitude + 1` (the positive and negative extent of the curve, plus a pixel for the axis).
