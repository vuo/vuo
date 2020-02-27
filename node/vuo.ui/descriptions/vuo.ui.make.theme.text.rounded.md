Create a new rounded theme to be used when rendering text fields.

The rounded theme is an easy to customize built-in theme with beveled edges.  If no theme is provided to a UI node's theme port, this is the theme that will be used.

- `Minimum Width / Height` — The minimum width and height that this text field can be.  A text field's width and height are determined by the size of the label text bounds plus padding, or the minimum width/height, depending on which is larger.
- `Label Font` — The font to use when rendering the text field's label.
- `Placeholder Font` — The font to use when rendering the text field's placeholder text.
- `Label Anchor` — How to align the text relative to the text field.
- `Label Padding` — Additional space to be added to the label on the horizontal and vertical axes.  Padding is in Vuo coordinates (0 is no extra space, 2 is the entire window width).
- `Color` — The color to apply to the background of the text field.
- `Hovered Color` — The color to apply to the background of the text field when an input device is hovering over the text field.
- `Pressed Color` — The color to apply to the background when an input device is pressing on the text field.
- `Border Color` — The color to apply to the border of the text field.
- `Border Thickness` — The size of the border strip around the text field.  This value is relative to the size of the text field (ex, 0 is no border, .5 is half the total size of the text field).
- `Corner Radius` — How rounded the corners of the rectangle are.  A value of 0 means the corners are sharp, producing a rectangle; a value of 1 means the corners are fully rounded, producing a circle (if the width and height are equal) or a capsule (if the width and height differ).
