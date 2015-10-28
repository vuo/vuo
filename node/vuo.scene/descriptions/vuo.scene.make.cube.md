Creates a cube that can be added to a 3D scene.

The cube has edges of length 1 (in Vuo coordinates) and is centered in the scene, unless the `Transform` input is used.

   - `Transform` — A transform that changes the cube's translation, rotation, or scale. It should use Vuo coordinates.
   - `Front Shader`, `Left Shader`, `Right Shader`, `Back Shader`, `Top Shader`, `Bottom Shader` — A shader to determine how the face of the cube will be drawn, such as lighting and color. If no shader is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the cube face.
