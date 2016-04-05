Creates a color from a hexadecimal color code.

This node accepts hexadecimal color codes in the following formats:

   - `#rgb` — 4-bit red, green, and blue values
   - `#rgbo` — 4-bit red, green, blue, and opacity values
   - `#rrggbb` — 8-bit red, green, and blue values
   - `#rrggbboo` — 8-bit red, green, blue, and opacity values

A hexadecimal color code is a concise way to represent the red, green, blue, and optionally opacity values of a color.  It begins with the pound sign `#`, and includes either a 4-bit or 8-bit hexadecimal representation of each color channel.

A 4-bit hexadecimal digit is `0` through `9` or `a` through `f` (which represent decimal values 10 through 15).

Likewise, an 8-bit hexadecimal digit is `00` through `ff`, representing decimal numbers 0 through 255.

For example:

   - The hexadecimal color code `#f00` represents opaque red — the red channel is `f` (the maximum), the green and blue channels are `0` (the minimum), and the opacity is unspecified and is assumed to be fully-opaque.
   - The hexadecimal color code `#ff00007f` represents semitransparent red — the last two characters, `7f`, represent opacity, with decimal value 127 (halfway between 0 and 255).
