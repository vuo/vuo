Creates a circular gradient that transitions smoothly between the given colors.

   - `colors` — The colors to transition between, in order from center outward.
   - `gradientCenter` — The center point, where the first color appears, in Vuo coordinates.
   - `gradientRadius` — The distance from the center to where the next-to-last color finishes transitioning to the last color, in Vuo coordinates.
   - `layerCenter` — The center point of the output layer, in Vuo coordinates.
   - `layerRotation` — The output layer's rotation to the left, in degrees.
   - `layerWidth` — The output layer's width, in Vuo coordinates.
   - `layerHeight` — The output layer's height, in Vuo coordinates.

Beyond `gradientRadius`, the image is filled with the last color.
