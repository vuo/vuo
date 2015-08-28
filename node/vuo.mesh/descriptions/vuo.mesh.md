These nodes are for working with 3D shapes defined by meshes.

A **mesh** is a set of vertices (points in 3D space) that define a shape, along with additional information about how the shape should be rendered. The additional information includes: the way that the vertices should be arranged into points, lines, or triangles (*elements* and *element assembly method*); the way that a texture should be stretched across the vertices (*texture coordinates*); and the way that the shape should respond to lighting (*normals*, *tangents*, and *bitangents*).

To create a 3D object from a mesh, you can use the `Make 3D Object` node in the `vuo.scene` node set. Then use the `Render Scene to Window` node to display it.

The `vuo.scene` node set provides additional nodes for working with meshes. To load a mesh from a file, you can use the `Get Scene` node. As a shortcut for creating 3D objects based on simple meshes, you can use nodes such as `Make Cube` and `Make 3D Object from Image`. 
