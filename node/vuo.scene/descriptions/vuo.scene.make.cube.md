Creates a scene object in the shape of a cube.

   - `Transform` — A transform that changes the cube's translation, rotation, or scale. It should use Vuo coordinates.
   - `Front Shader`, `Left Shader`, `Right Shader`, `Back Shader`, `Top Shader`, `Bottom Shader` — A shader to determine how the face of the cube will be drawn, such as lighting and color. If no shader is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the cube face.

By default, the cube has a width, height, and depth of 1 and is centered at (0,0,0).

For a simpler version of this node, where all 6 sides of the cube have the same material, use `Make Cube`.
