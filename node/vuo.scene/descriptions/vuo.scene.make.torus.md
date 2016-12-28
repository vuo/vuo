Creates a scene object in the shape of a torus.

- `Transform` — A transform that changes the torus's translation, rotation, or scale. It should use Vuo Coordinates.
- `Material` — A shader, image, or color to apply to the torus. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the surface of the torus.
- `Rows`/`Columns` — The number of vertices along the torus's latitude/longitude. With more vertices, the torus has a rounder shape but may take longer to render.
- `Thickness` – The diameter of the tube that makes up the torus ring.  A value of 1 means that the insides of the tube touch at the center, where a smaller value will produce a hole in the middle.

By default, the torus has a diameter of 1 (spanning the outer edges) and is centered at (0,0,0).
