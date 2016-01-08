Creates a graphics shader that can paint colors on a 3D object representing its vertex attributes.

This node is useful for debugging meshes.

  - `Attribute` — Which vertex attribute to render as a color:
    - `Position` — Use the world-space position of each of the object's vertices.
    - `Normal` — Use the world-space normal vector of each of the object's vertices.  A _normal vector_ represents the direction a 3D surface is facing.
    - `Tangent` — Use the world-space tangent vector of each of the object's vertices.  A _tangent vector_ represents a direction along a 3D surface.
    - `Bitangent` — Use the world-space bitangent vector of each of the object's vertices.  A _bitangent vector_ represents a direction along a 3D surface, perpendicular to the tangent vector.
    - `Texture Coordinate` — Render a checkerboard using the X and Y texture coordinate values.
  - `X Color`, `Y Color`, `Z Color` — The color that's used to represent the X, Y, and Z axis.

With its default settings, this node will render parts of an object:

   - red if they face leftward (-X) or rightward (+X)
   - green if they face upward (+Y) or downward (-Y)
   - blue if they face forward (+Z) or backward (-Z)
   - or a mixture of the above colors if they face in between
