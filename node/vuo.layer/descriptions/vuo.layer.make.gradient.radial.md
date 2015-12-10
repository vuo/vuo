Creates a circular gradient that transitions smoothly between the given colors.

   - `Name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `Colors` — The colors to transition between, in order from center outward.
   - `Gradient Center` — The center point, where the first color appears, in Vuo coordinates relative to a layer whose corners are at (-1, -1) and (1, 1).
   - `Gradient Radius` — The distance from the center to where the next-to-last color finishes transitioning to the last color, in Vuo coordinates relative to a layer whose corners are at (-1, -1) and (1, 1).
   - `Layer Center` — The center point of the output layer, in Vuo coordinates.
   - `Layer Rotation` — The output layer's rotation counterclockwise, in degrees.
   - `Layer Width` — The output layer's width, in Vuo coordinates.
   - `Layer Height` — The output layer's height, in Vuo coordinates.

The values of `Gradient Center` and `Gradient Radius` are based on a -1 to 1 range along the X and Y axes. Imagine that the gradient is drawn on a layer whose corners are at (-1, -1) and (1, 1), and then the layer is scaled to match `Layer Width` and `Layer Height`. For example, if `Gradient Radius` is 1, then the gradient always extends to the edge of the layer — even if the output layer ends up being scaled so that the distance from center to edge is no longer 1.

Beyond `Gradient Radius`, the image is filled with the last color.
