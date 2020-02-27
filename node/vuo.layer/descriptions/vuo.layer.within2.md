Outputs *true* if the point is within the given layer.

This node checks the point against the layer's position within the rendered composite image. For example, if a layer is scaled and rotated before being rendered, then this node checks if the point falls within the layer's scaled and rotated bounds.

   - `Point` — The point to check, in Vuo Coordinates.
   - `Layer` — The layer to check the point against.
   - `Window` — The window in which the layer is rendered.
   - `Within Layer` — *True* if the layer is in the rendered composite image and the point is in the layer.
