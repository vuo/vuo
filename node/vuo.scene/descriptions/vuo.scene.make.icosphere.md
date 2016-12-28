Creates a scene object in the shape of a sphere.

An Icosphere is a sphere mesh that distributes vertices more evenly than its counterpart, the UV sphere.  This makes it a good match for use in 3D filters, as the deformations will be smoother.

- `Transform` — A transform that changes the sphere's translation, rotation, or scale. It should use Vuo Coordinates.
- `Material` — A shader, image, or color to apply to the sphere. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the surface of the sphere.
- `Subdivisions` — Splits each triangle into 4 smaller triangles, to form a closer approximation of a sphere.  At 0, this node outputs an icosahedron (20 triangles).  At 1, it outputs 80 triangles.  At 2: 320, and so on.

By default, the sphere has a diameter of 1 and is centered at (0,0,0).
