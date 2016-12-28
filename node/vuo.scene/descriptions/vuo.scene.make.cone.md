Creates a scene object in the shape of a cone.

   - `Transform` — A transform that changes the cone's translation, rotation, or scale. It should use Vuo Coordinates.
   - `Material` — A shader, color, or image to draw as the cone's texture.  If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the surfaces of the cone.
   - `Columns` — How many segments make up this cone.  More columns means smoother edges.

By default, the cone has a diameter and height of 1 and is centered at (0,0,0).
