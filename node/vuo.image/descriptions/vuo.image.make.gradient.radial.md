Creates a circular gradient that transitions smoothly between the given colors.

   - `colors` — The colors to transition between, in order from center outward.
   - `center` — The center point, where the first color appears, in Vuo coordinates.
   - `radius` — The distance from the center to where the next-to-last color finishes transitioning to the last color, in Vuo coordinates.
   - `width` — The output image's width, in pixels.
   - `height` — The output image's height, in pixels.

Beyond `radius`, the image is filled with the last color.
