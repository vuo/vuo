Creates a scene object made up of points.

   - `Transform` — Changes the object's translation, rotation, or scale, in Vuo Coordinates.
   - `Material` — A shader, image, or color to apply to the object. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the object.
   - `Point Size` — How wide and high to draw the points, in Vuo Coordinates.
   - `Positions` — A list containing the 2D or 3D location of each point.
   - `Colors` — A list specifying the color of each point.  This color is multiplied by the Material's color.  If the number of colors differs from the number of points, the colors are spread across the points.

When rendered, this object will appear as a set of points without lines or surfaces connecting them — not as a solid 3D object.
