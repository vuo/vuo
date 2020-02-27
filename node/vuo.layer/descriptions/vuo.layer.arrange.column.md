Moves layers so that they are arranged in a vertical column.

The position of the arranged layers is based on the bounding rectangle around the layers — the smallest rectangle that can enclose them. The bounding rectangle is always aligned with the X-axis and Y-axis (not rotated).

   - `Layers` — The layers to arrange, in order from top to bottom.
   - `Window` — The window in which the layers are rendered.  This is only necessary when working with Real Size layers.
   - `Horizontal Alignment` — The way the arranged layers line up with each other: by their left-most, right-most, or center points.
   - `Anchor` — The point within the arranged layers' bounding rectangle that should be fixed at `Position`.
   - `Position` — The point within the composite image where the arranged layers should be placed, in Vuo Coordinates.
   - `Spacing` — The distance between adjacent layers, in Vuo Coordinates.
   - `Arranged Layer` — A parent layer containing the arranged layers as children. (`Arranged Layer` omits empty layers, for example if one of the `Layers` drawer ports doesn't have a cable connected.)
