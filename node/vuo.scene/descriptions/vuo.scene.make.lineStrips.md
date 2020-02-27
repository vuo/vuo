Creates a scene object made up of a series of connected line segments.

   - `Transform` — Changes the object's translation, rotation, or scale, in Vuo Coordinates.
   - `Material` — A shader, image, or color to apply to the object. If no material is provided, this node uses a default shader that stretches a gradient-colored checkerboard across the object.
   - `Line Width` — How wide to draw the lines, in Vuo Coordinates.
   - `Positions` — A list of the endpoints of the line segments. The 1st point and 2nd point are endpoints for one line segment, the 2nd point and 3rd point are endpoints for another line segment, and so on.
   - `Colors` — A list specifying the color of each endpoint of each line segment.  This color is multiplied by the Material's color.  If the number of colors differs from the number of positions, the colors are spread across the positions.

Even if the line segments connect to form a 2D or 3D object, they're rendered simply as line segments, not as a solid object.
