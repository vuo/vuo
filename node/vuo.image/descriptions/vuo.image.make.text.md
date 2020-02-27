Renders an image containing the specified text.

   - `Text` — The text to render.
   - `Font` — The font used to render the text.  See also the [Make Font](vuo-node://vuo.font.make) node.
   - `Rotation` — The angle to rotate the text counterclockwise, in degrees.
   - `Wrap Width` — The width, in pixels, at which long lines of text should be word-wrapped.  If set to `Auto`, the image will be as wide as necessary to accommodate the longest line of text.  Otherwise, the image's width may be less than the Wrap Width (since text is wrapped a word at a time) or more than the Wrap Width (if the Wrap Width is less than the widest single character).
   - `Image` — An image containing the text. The image is sized to fit the text.

To enter multiple lines of text in the `Text` port's input editor, use Option-Return to insert line breaks.

If you need to resize the output image, consider using the [Make Text Layer](vuo-node://vuo.layer.make.text2) node instead. This will keep the text sharp as it is enlarged.
