Creates a text layer that can be combined with other layers to create a composite image.

The text will remain sharp as it scales with the composite image. The text will be rendered at pixel-perfect resolution.

   - `Text` — The text to render.
   - `Font` — The font used to render the text.  See also the [Make Font](vuo-node://vuo.font.make) node.  The font size you specify here is the size, in points, at which the text would be rendered in a 1024-point-wide window or image. If the actual window or image containing the text layer is smaller or larger, the text is scaled proportionately.
   - `Anchor` — The point within the layer that should be fixed at `Position`.
   - `Position` — The point within the composite image where the layer should be placed, in Vuo Coordinates. The position is snapped so that it aligns exactly with a pixel.
   - `Rotation` — The angle to rotate the text counterclockwise, in degrees.
   - `Wrap Width` — The width, in Vuo Coordinates, at which long lines of text should be word-wrapped.  If set to `Auto`, the layer will be as wide as necessary to accommodate the longest line of text.  Otherwise, the layer's width may be less than the Wrap Width (since text is wrapped a word at a time) or more than the Wrap Width (if the Wrap Width is less than the widest single character).
   - `Opacity` — The text's opacity, from 0 (fully transparent) to 1 (same opacity as the `Font`'s color).
   - `Layer` — A layer containing the text. The layer is sized to fit the text.

To stretch or compress the text, use [Transform Layer](vuo-node://vuo.layer.transform).

To create text that stays the same size rather than scaling with the composite image, use [Make Text Image](vuo-node://vuo.image.make.text) and [Make Image Layer (Real Size)](vuo-node://vuo.layer.make.realSize2).
