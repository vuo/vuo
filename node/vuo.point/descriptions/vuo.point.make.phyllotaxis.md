Calculates a list of points in a phyllotaxis pattern, like the scales in a pinecone or the leaves in an aloe plant.

   - `Center` — The center position of the spiral, in Vuo Coordinates.
   - `Radius` — The distance from the center of the spiral to the outermost point, in Vuo Coordinates.
   - `Number of Points` — How many points to create.
   - `Start Point` — Whether to start at the center point (when `Start Point` is 1) or a point further out on the spiral.
   - `Angle` — The amount each new point is moved relative to the previous point, in degrees.  The default value, 137.5°, is approximately the [golden angle](https://mathworld.wolfram.com/GoldenAngle.html).

Thanks to [Martinus Magneson](https://community.vuo.org/u/MartinusMagneson) for developing the node this is based on.
