Creates a scene object in the shape of a square.

- `Transform` — A transform that changes the square's translation, rotation, or scale. It should use Vuo Coordinates.
- `Material` — A shader, image, or color to apply to the square. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the square.
- `Rows`/`Columns` — The number of vertices along the square's latitude/longitude. With more vertices, the square can be deformed more flexibly by object filter nodes, but may take longer to render.

By default, the square is on the XY plane, has a width and height of 1, and is centered at (0,0,0).
