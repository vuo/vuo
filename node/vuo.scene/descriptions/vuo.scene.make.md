Creates a 3D object that can be added to a 3D scene.

   - `vertices` — The vertices (points in 3D space) that define the shape of the object. These should use Vuo coordinates. For this node to create a 3D object, you must give it some vertices.
   - `shader` — A shader that determines how the vertices will be drawn, such as lighting and color. If no shader is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the vertices.
   - `transform` — A transform that changes the 3D object's translation, rotation, or scale. It should use Vuo coordinates.
