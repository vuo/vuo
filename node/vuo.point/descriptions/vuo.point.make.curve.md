Calculates a list of points along an easing curve.

This node can be used to replicate an object along a curved path. Create a list of points, and then use the [Copy Layer](vuo-node://vuo.layer.copy) or [Copy 3D Object](vuo-node://vuo.scene.copy) node to create multiple instances of an object.

   - `Start Position` — The first position to output.
   - `End Position` — The last position to output.
   - `Curve` — The shape of the curve.
      - Linear — Produces points in a line.
      - Quadratic — Produces a gradual curve.
      - Cubic — Produces a slightly steeper curve.
      - Circular — Produces a curve that traces along a circle.
      - Exponential — Produces a steep curve.
   - `Easing` — Which part of the curve is shallow.
      - In — The curve is shallowest near `Start Position` and steepest near `End Position`.
      - Out — The curve is shallowest near `End Position` and steepest near `Start Position`.
      - In + Out — The curve is shallowest near `Start Position` and `End Position` and steepest near the middle.
      - Middle — The curve is shallowest in the middle and steepest near `Start Position` and `End Position`.
   - `Number Of Points` — How many points to create.
