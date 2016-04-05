Renders an image containing the specified text.

   - `Text` — The text to render.
   - `Font` — The font used to render the text.  See also the `Make Font` node.
   - `Image` — The image containing the rendered text.  The image is automatically sized to fit the specified text.

To enter multiple lines of text in the `Text` port's input editor, use Option-Return to insert line breaks.

If the output image is sent to a node that enlarges the image, then the text may become blurry. To keep the text sharp, either avoid using nodes that enlarge the image, or else choose a larger font size so that the output image is larger. For example, if creating a layer from the image, either use the `Make Layer` or `Make Layer with Shadow` nodes to keep the image at its original size, or else choose a larger font size and use the `Make Scaled Layer` or `Make Scaled Layer with Shadow` nodes.
