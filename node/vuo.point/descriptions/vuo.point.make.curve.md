Calculates a list of points along an easing curve.

This node can be used to replicate an object along a curved path. Create a list of points, and then use the `Copy Layer` or `Copy 3D Object` node to create multiple instances of an object.

   - `startPosition` — The first position to output.
   - `endPosition` — The last position to output.
   - `curve` — The shape of the curve.
      - Linear — Produces points in a line.
      - Quadratic — Produces a gradual curve.
      - Cubic — Produces a slightly steeper curve.
      - Circular — Produces a curve that traces along a circle.
      - Exponential — Produces a steep curve.
   - `easing` — Which part of the curve is shallow.
      - In — The curve is shallowest near `startPosition` and steepest near `endPosition`.
      - Out — The curve is shallowest near `endPosition` and steepest near `startPosition`.
      - In + Out — The curve is shallowest near `startPosition` and `endPosition` and steepest near the middle.
      - Middle — The curve is shallowest in the middle and steepest near `startPosition` and `endPosition`.
   - `numberOfPoints` — How many points to create.
