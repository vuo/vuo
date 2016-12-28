Creates a square mesh made up of points.

- `Transform` — A transform that changes the grid's translation, rotation, or scale. It should use Vuo Coordinates.
- `Material` — A shader, image, or color to apply to the points. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the points.
- `Rows`/`Columns` — The number of points along the grid's latitude/longitude.
- `Point Size` — The width and height of each point in the grid, in Vuo Coordinates.

By default, the grid is on the XY plane, has a width and height of 1, and is centered at (0,0,0).
