Creates a circular gradient that transitions smoothly between the given colors.

   - `Colors` — The colors to transition between, in order from center outward.
   - `Center` — The center point, where the first color appears, in Vuo Coordinates.
   - `Radius` — The distance from the center to where the next-to-last color finishes transitioning to the last color, in Vuo Coordinates.
   - `Noise Amount` — How much random noise to add to the gradient.  A small amount of noise can help gradients look smoother.
   - `Width` — The output image's width, in pixels.
   - `Height` — The output image's height, in pixels.

Beyond `Radius`, the image is filled with the last color.
