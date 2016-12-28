Creates a square mesh made up of lines.

- `Transform` — A transform that changes the grid's translation, rotation, or scale. It should use Vuo Coordinates.
- `Material` — A shader, image, or color to apply to the lines. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the lines.
- `Rows`/`Columns` — The number of lines along the grid's latitude/longitude.
- `Grid Type` — Whether to include horizontal lines, vertical lines, or both.
- `Line Width` — The width of each line in the grid, in Vuo Coordinates.

By default, the grid is on the XY plane, has a width and height of 1, and is centered at (0,0,0).
