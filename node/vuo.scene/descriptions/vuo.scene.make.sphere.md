Creates a scene object in the shape of a sphere.

- `Transform` — Changes the sphere's translation, rotation, or scale, in Vuo Coordinates.
- `Material` — A shader, image, or color to apply to the sphere. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the surface of the sphere.
- `Rows`/`Columns` — The number of vertices along the sphere's latitude/longitude. With more vertices, the sphere has a rounder shape but may take longer to render.

By default, the sphere has a diameter of 1 and is centered at (0,0,0).
