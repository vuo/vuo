Creates a layer that contains text and/or an icon, and fires an event when it's clicked.

   - `Rendered Layers` — The group of rendered layers containing the layer. Connect this to the [Render Layers to Window](vuo-node://vuo.layer.render.window) node's `Rendered Layers` output port.
   - `Label` — The text to show on the button.
   - `Font` — The font to use for the button's text.
   - `Color` — The background color of the button.
   - `Icon` — An optional icon to show on the button.
   - `Icon Positon` — How to place the icon relative to the text.  When this is Left, Right, Above, or Below, the icon is scaled to fit within the space that remains after rendering the Label.  When this is Behind, the icon is scaled to fit within the button.
   - `Center`, `Width`, `Height` — The button's position and size, in Vuo Coordinates.
