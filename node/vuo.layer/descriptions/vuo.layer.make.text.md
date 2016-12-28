Creates a layer that can be combined with other layers to create a composite image. 

This node makes it easy to create sharp text — it always renders text at pixel-perfect resolution.  The layer is always rendered at its original size and rotation in the composite image — if you want to scale and rotate the layer, use the `Make Text Image` and `Make Scaled Layer` nodes instead.

   - `Text` — The text to render.
   - `Font` — The font used to render the text.  See also the `Make Font` node.
   - `Center` — The center point of the text, in Vuo Coordinates.  The center point is snapped so that it aligns exactly with a pixel.
