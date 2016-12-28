Creates a 3D object that can be added to a 3D scene.

   - `Mesh` — The mesh that defines the shape of the object. Its vertices should use Vuo Coordinates. For this node to create a 3D object, you must give it a mesh.
   - `Shader` — A shader that determines how the vertices will be drawn, such as lighting and color. If no shader is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the vertices.
   - `Transform` — A transform that changes the 3D object's translation, rotation, or scale. It should use Vuo Coordinates.
