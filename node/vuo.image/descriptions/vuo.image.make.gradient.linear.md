Creates a gradient that transitions smoothly between the given colors along a straight line. 

   - `colors` — The colors to transition between, in order from start to end.
   - `start` - The point where the first color begins transitioning to the next color, in Vuo coordinates. 
   - `end` - The point where the next-to-last color finishes transitioning to the last color, in Vuo coordinates.
   - `width` — The output image's width, in pixels.
   - `height` — The output image's height, in pixels.

If `start` is directly above `end`, then the colors transition from top to bottom. If `start` is directly to the left of `end`, then the colors transition from left to right. If `start` is diagonal from `end`, then the colors transition diagonally. 

Beyond `start` and `end`, the image is filled with the first and last color (or, if `start` and `end` are the same, just the first color). 
