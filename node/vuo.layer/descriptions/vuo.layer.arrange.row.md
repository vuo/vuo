Moves layers so that they are arranged in a horizontal row.

The position of the arranged layers is based on the bounding rectangle around the layers — the smallest rectangle that can enclose them. The bounding rectangle is always aligned with the X-axis and Y-axis (not rotated).

   - `Layers` — The layers to arrange, in order from left to right.
   - `Rendered Layers` — The group of rendered layers containing the arranged layers.  This is only necessary when working with Real Size layers.
   - `Vertical Alignment` — The way the arranged layers line up with each other: by their top-most, bottom-most, or center points.
   - `Anchor` — The point within the arranged layers' bounding rectangle that should be fixed at `Position`.
   - `Position` — The point within the composite image where the arranged layers should be placed, in Vuo Coordinates.
   - `Spacing` — The distance between adjacent layers, in Vuo Coordinates.
   - `Arranged Layer` — A parent layer containing the arranged layers as children. (`Arranged Layer` omits empty layers, for example if one of the `Layers` drawer ports doesn't have a cable connected.)
