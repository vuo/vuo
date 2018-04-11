Outputs the center and size of a layer.

This node finds the center and size of the layer's bounding rectangle — the smallest rectangle that can enclose the layer. The bounding rectangle is always aligned with the X-axis and Y-axis — it's never rotated. The bounding rectangle uses the layer's sharp edges. Blurry edges may extend beyond the bounding rectangle.

   - `Rendered Layers` — The group of rendered layers containing the layer.
   - `Layer Name` — The name of the layer.
   - `Include Children` — Whether the output bounds include the bounds of any child layers.
   - `Center`, `Width`, `Height` — The dimensions of the layer's bounding rectangle, in Vuo Coordinates.
   - `Pixels Wide`, `Pixels High` — The dimensions of the layer's bounding rectangle, in pixels.
