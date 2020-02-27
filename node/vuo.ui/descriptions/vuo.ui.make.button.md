Creates a layer that fires an event when clicked.

   - `Window` — The window in which the layer is rendered. Connect this to the [Render Layers to Window](vuo-node://vuo.layer.render.window2) node's `Updated Window` output port.
   - `Label` — The text to show on the button.
   - `Anchor` — The point within the layer that should be fixed at `Position`.  For example, if `Anchor` is Top Left, `Position` represents the top left corner of a rectangle circumscribing the button.
   - `Position` — The button's position, in Vuo Coordinates.
   - `Theme` — Information about the button's appearance.  See the [Make Action Button Theme (Rounded)](vuo-node://vuo.ui.make.theme.button.rounded) node.

The button's size is automatically determined from the size of the `Label` text, limited by the minimum width and height specified by the `Theme`.
