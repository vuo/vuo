Creates a circular gradient that transitions smoothly between the given colors.

   - `Colors` — The colors to transition between, in order from center outward.
   - `Center` — The center point, where the first color appears, in Vuo coordinates.
   - `Radius` — The distance from the center to where the next-to-last color finishes transitioning to the last color, in Vuo coordinates.
   - `Width` — The output image's width, in pixels.
   - `Height` — The output image's height, in pixels.

Beyond `Radius`, the image is filled with the last color.
