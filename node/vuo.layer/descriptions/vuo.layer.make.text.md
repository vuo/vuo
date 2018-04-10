Creates a layer that can be combined with other layers to create a composite image.

The layer created by this node will keep the text sharp. The text will be rendered at pixel-perfect resolution.

Since this node creates a Real Size layer, the layer will be rendered at its original size and rotation in the composite image. If you want to scale and rotate the layer, use the `Make Text Image` and `Make Image Layer (Scaled)` nodes instead.

   - `Text` — The text to render.
   - `Font` — The font used to render the text.  See also the `Make Font` node.
   - `Anchor` — The point within the layer that should be fixed at `Position`.
   - `Position` — The point within the composite image where the layer should be placed, in Vuo Coordinates. The position is snapped so that it aligns exactly with a pixel.
