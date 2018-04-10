Creates copies of a layer tiled in a grid.

   - `Layer` — The layer to copy.
   - `Center` — The position where the corners of the center 4 grid areas touch, in Vuo Coordinates.
   - `Spacing` — The distance between the center of each tiled layer, in Vuo Coordinates.
   - `Field Size` — The extent of the grid, in Vuo Coordinates.

When `Center` changes, layers that pass entirely outside the `Field Size` area disappear, and reappear on the opposite side.  By animating `Center` and creating a field large enough to encompass the viewport, you can create the illusion of traveling through an infinite series of layers.
