Creates a solid-color oval layer that can be combined with other layers to create a composite image.

   - `Color` — The color that fills the oval.
   - `Anchor` — The point within the layer that should be fixed at `Position`. For example, if `Anchor` is Top Left, `Position` represents the top left corner of a rectangle circumscribing the oval.
   - `Position` — The point within the composite image where the layer should be placed, in Vuo Coordinates.
   - `Rotation` — The oval's rotation counterclockwise, in degrees.
   - `Width` — The width of the oval, in Vuo Coordinates.
   - `Height` — The height of the oval, in Vuo Coordinates.
   - `Sharpness` — How sharp the edge of the oval is.  A value of 0 means the edge is blurry; a value of 1 means the edge is sharp.  The blur extends beyond `Width` and `Height`.
   - `Opacity` — The oval's opacity, from 0 (fully transparent) to 1 (same opacity as the input `Color`).
