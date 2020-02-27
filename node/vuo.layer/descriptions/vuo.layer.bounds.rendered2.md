Outputs the center and size of a layer as it is actually rendered.

This node finds the center and size of the layer's bounding rectangle — the smallest rectangle that can enclose the layer.  The bounding rectangle is calculated after applying the entire stack of transformations, and is always aligned with the X-axis and Y-axis — it's never rotated.

The bounding rectangle uses the layer's sharp edges.  (Blurry edges may extend beyond the bounding rectangle.)

   - `Layer` — Which layer's bounds to find.
   - `Window` — The window in which the layer is rendered.  Connect this to the `Updated Window` output of [Render Layers to Window](vuo-node://vuo.layer.render.window2).
   - `Include Children` — Whether the output bounds include the bounds of any child layers.
   - `Center`, `Width`, `Height` — The dimensions of the layer's bounding rectangle, in Vuo Coordinates.
   - `Pixels Wide`, `Pixels High` — The dimensions of the layer's bounding rectangle, in pixels.

See also [Get Layer Bounds](vuo-node://vuo.layer.bounds), which can output the bounds of partially-transformed layers and does not require a `Window` input.
