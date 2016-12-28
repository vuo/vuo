Creates a 3D object made up of points at random positions.

   - `Transform` — By default, the points are created within a 1-unit-wide shape centered at the origin; this port changes that shape's translation, rotation, and scale.
   - `Material` — A shader, image, or color to apply to the points.  Each point is assigned random texture coordinates.  If no material is provided, this node uses a default gradient-colored checkerboard shader.
   - `Distribution` — The shape in which the random points are placed.
   - `Count` — The number of points to create.
   - `Point Size` — The width and height of each point, in Vuo Coordinates.
   - `Seed` — The sequence of pseudorandom numbers to use.
