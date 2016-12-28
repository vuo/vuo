Creates a gradient that transitions smoothly between the given colors along a straight line. 

   - `Colors` — The colors to transition between, in order from start to end.
   - `Start` - The point where the first color begins transitioning to the next color, in Vuo Coordinates. 
   - `End` - The point where the next-to-last color finishes transitioning to the last color, in Vuo Coordinates.
   - `Noise Amount` — How much random noise to add to the gradient.  A small amount of noise can help gradients look smoother.
   - `Width` — The output image's width, in pixels.
   - `Height` — The output image's height, in pixels.

If `Start` is directly above `End`, then the colors transition from top to bottom. If `Start` is directly to the left of `End`, then the colors transition from left to right. If `Start` is diagonal from `End`, then the colors transition diagonally. 

Beyond `Start` and `End`, the image is filled with the first and last color (or, if `Start` and `End` are the same, just the first color). 
