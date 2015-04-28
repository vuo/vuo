Creates a gradient that transitions smoothly between the given colors along a straight line. 

   - `name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `colors` — The colors to transition between, in order from start to end.
   - `gradientStart` - The point where the first color begins transitioning to the next color, in Vuo coordinates. 
   - `gradientEnd` - The point where the next-to-last color finishes transitioning to the last color, in Vuo coordinates.
   - `layerCenter` — The center point of the output layer, in Vuo coordinates.
   - `layerRotation` — The output layer's rotation to the left, in degrees.
   - `layerWidth` — The output layer's width, in Vuo coordinates.
   - `layerHeight` — The output layer's height, in Vuo coordinates.

If `gradientStart` is directly above `gradientEnd`, then the colors transition from top to bottom. If `gradientStart` is directly to the left of `gradientEnd`, then the colors transition from left to right. If `gradientStart` is diagonal from `gradientEnd`, then the colors transition diagonally. 

Beyond `gradientStart` and `gradientEnd`, the image is filled with the first and last color (or, if `gradientStart` and `gradientEnd` are the same, just the first color). 
