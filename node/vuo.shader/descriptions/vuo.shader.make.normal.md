Creates a graphics shader that can paint colors on a 3D object representing its vertex normals.

This node is useful for debugging meshes.

A _normal vector_ represents the direction a 3D surface is facing; a _vertex normal_ is the surface's direction at a particular vertex.  With its default settings, this node will render parts of an object:

   - red if they face leftward (-X) or rightward (+X)
   - green if they face upward (+Y) or downward (-Y)
   - blue if they face forward (+Z) or backward (-Z)
   - or a mixture of the above colors if they face in between
