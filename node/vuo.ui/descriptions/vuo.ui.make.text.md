Creates a layer that provides a field into which the user can enter a single line of text.

   - `Rendered Layers` — The group of rendered layers containing the layer. Connect this to the [Render Layers to Window](vuo-node://vuo.layer.render.window) node's `Rendered Layers` output port.
   - `Label` — The text to show next to the text field.
   - `Placeholder` — The text to show in the text field when no user input is entered.
   - `Position` — The text field's position, in Vuo Coordinates.  Width and height will be automatically determined from the size of the rendered text and the minimum width and height as set by the theme.
   - `Anchor` — How to align the text field relative to the `Position`, both horizontally and vertically.
   - `Theme` — The theme to use when rendering the text field.
