Creates a layer that provides a field into which the user can enter a single line of text.

   - `Window` — The window in which the layer is rendered. Connect this to the [Render Layers to Window](vuo-node://vuo.layer.render.window2) node's `Updated Window` output port.
   - `Set Value` — Sets the initial text value, or changes its current text value.  If empty, the field will show the `Placeholder Text`.
   - `Placeholder Text` — The text to show in the field when no input has been entered.
   - `Anchor` — The point within the layer that should be fixed at `Position`.  For example, if `Anchor` is Top Left, `Position` represents the top left corner of the field's outer rectangle.
   - `Position` — The field's position, in Vuo Coordinates.
   - `Field Width` — The size of the text entry part of the widget (not including padding or border), in Vuo Coordinates.  The width will expand, if needed, to accommodate the text value or placeholder text.  The height is automatically determined from the line height of the theme's font.
   - `Theme` — Information about the field's appearance.  See the [Make Text Field Theme (Rounded)](vuo-node://vuo.ui.make.theme.text.rounded) node.
   - `Updated Value` — Fires with the initial text (if non-empty), and when the user finishes editing the text (by clicking outside the text field, or pressing Return, Enter, Tab, or Esc) if the text changed.
