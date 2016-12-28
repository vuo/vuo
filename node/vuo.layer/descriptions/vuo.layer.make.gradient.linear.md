Creates a gradient that transitions smoothly between the given colors along a straight line. 

   - `Name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `Colors` — The colors to transition between, in order from start to end.
   - `Gradient Start` — The point where the first color begins transitioning to the next color, in Vuo Coordinates relative to a layer whose corners are at (-1, -1) and (1, 1). 
   - `Gradient End` — The point where the next-to-last color finishes transitioning to the last color, in Vuo Coordinates relative to a layer whose corners are at (-1, -1) and (1, 1).
   - `Gradient Noise Amount` — How much random noise to add to the gradient.  A small amount of noise can help gradients look smoother.
   - `Layer Center` — The center point of the output layer, in Vuo Coordinates.
   - `Layer Rotation` — The output layer's rotation counterclockwise, in degrees.
   - `Layer Width` — The output layer's width, in Vuo Coordinates.
   - `Layer Height` — The output layer's height, in Vuo Coordinates.

The values of `Gradient Start` and `Gradient End` are based on a -1 to 1 range along the X and Y axes. Imagine that the gradient is drawn on a layer whose corners are at (-1, -1) and (1, 1), and then the layer is scaled to match `Layer Width` and `Layer Height`. For example, if `Gradient Start` is at (-1, -1), then the gradient always starts at the bottom left of the layer — even if the output layer ends up being scaled so that its bottom left is no longer (-1, -1).

If `Gradient Start` is directly above `Gradient End`, then the colors transition from top to bottom. If `Gradient Start` is directly to the left of `Gradient End`, then the colors transition from left to right. If `Gradient Start` is diagonal from `Gradient End`, then the colors transition diagonally. 

Beyond `Gradient Start` and `Gradient End`, the image is filled with the first and last color (or, if `Gradient Start` and `Gradient End` are the same, just the first color). 
