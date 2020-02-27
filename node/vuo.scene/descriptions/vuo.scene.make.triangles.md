Creates a scene object made up of triangles.

   - `Transform` — Changes the object's translation, rotation, or scale, in Vuo Coordinates.
   - `Material` — A shader, image, or color to apply to the triangle. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the object.
   - `Texture Mapping` — How to assign texture coordinates to the vertices:
      - "Cubic" — Imagine a cube placed around the object.  The triangles visible from each face of the cube are given texture coordinates along that face of the cube.
      - "Spherical" — Imagine a sphere placed around the object.  The triangles are given texture coordinates by "shrinkwrapping" that sphere onto the object.
   - `Positions` — A list of the vertices of the triangles.  The 1st, 2nd, and 3rd points are part of the first triangle; the 4th, 5th, and 6th points are part of the second triangle, and so on.
   - `Colors` — A list specifying the color of each vertex.  This color is multiplied by the Material's color.  If the number of colors differs from the number of positions, the colors are spread across the positions.

The vertices of each triangle must be arranged counterclockwise (e.g., bottom left, bottom right, top) in order to be visible by default.  Or if you pass the object through [Show Back of 3D Object](vuo-node://vuo.scene.back), vertices in either clockwise or counterclockwise order will be visible.

Normals, tangents, and bitangents are calculated automatically.
