Outputs the center and size of a layer.

This node finds the center and size of the layer's bounding rectangle — the smallest rectangle that can enclose the layer.  The bounding rectangle is always aligned with the X-axis and Y-axis — it's never rotated.

The bounding rectangle is calculated to include the transformations up to the point where this node receives the layer (which does not necessarily represent how the layer appears when it is finally rendered).  Since the actual size and position of Real Size layers depends on the size of the rendering destination (which this node doesn't use), Real Size layers are not included in the calculation.  For Real Size layers, use [Get Rendered Layer Bounds](vuo-node://vuo.layer.bounds.rendered2).

The bounding rectangle uses the layer's sharp edges.  (Blurry edges may extend beyond the bounding rectangle.)

   - `Layer` — Which layer's bounds to find.
   - `Center`, `Width`, `Height` — The dimensions of the layer's bounding rectangle, in Vuo Coordinates.
