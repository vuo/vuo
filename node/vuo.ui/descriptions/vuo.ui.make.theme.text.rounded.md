Create a new rounded theme to be used when rendering text fields, text areas, and number fields.

   - `Font` — The font to use when rendering the field's text value and placeholder text.
   - `Text Anchor` — How to align the text relative to the field's background area.
   - `Text Padding` — Additional space to be added to the background on the horizontal and vertical axes, in Vuo Coordinates.
   - `Text Color`, `Hovered`, `Active` — The colors for the field's `Label` text, respectively, when not interacting with it, when the mouse is hovering over it, and when the field is focused.  (The `Font` also specifies a color; that color is multiplied by each of these colors.)
   - `Background Color`, `Hovered`, `Active` — The colors for the field's background area.
   - `Border Color`, `Hovered`, `Active` — The colors for the field's outline.
   - `Border Thickness` — The size of the field's outline, in Vuo Coordinates.
   - `Cursor Color` — The color of the text insertion caret.
   - `Selection Color` — The background color of selected text.
   - `Corner Roundness` — How rounded the corners of the field are.  A value of 0 means the corners are sharp, producing a rectangle; a value of 1 means the corners are fully rounded, producing a circle (if the width and height are equal) or a capsule (if the width and height differ).
