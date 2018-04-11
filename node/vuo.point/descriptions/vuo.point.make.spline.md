Adds more points in between the list of input points.

   - `Control Points` — A list of control points to interpolate between.
   - `Tween Point Count` — How many points each input point will be expanded into.  For example, when this is 16, each input point (except the last point) is expanded into 16 points.  When 1, no interpolation is performed, and the same list of points is output.
   - `Tension` — How sharply the curve bends at each control point.  At -1, the curve is slack and rounded.  At 1, the curve is tight and straight (linear).
   - `Continuity` — How sharp the transition between each segment of the curve is.  At -1, the transition is sharp.  At 0, the spline's tangent (first derivative) is continuous, resulting in smooth transitions.  At 1, the transition is sharp, and billows outward.
   - `Bias` — The direction of the curve as it passes through each control point.  At -1, the curve occurs before each control point, providing an undershooting, anticipatory behavior.  At 0, the curve is centered around each control point.  At 1, the curve occurs after each control point, providing a "follow through" behavior.

This node produces an approximation of a _cubic Hermite spline_.  The curve is continuous and passes through each control point (unlike, say, Bézier and B-splines, where the curve merely passes _near_ the control points).  The shape of the curve at a given point is affected by the point before and the point after it.

Changing the Tension and Continuity values has a similar effect on the path, but each differs in the way it affects the placement of the interpolated points:  increasing Tension causes the interpolated points to bunch up near the control points (resulting in smooth deceleration and acceleration when used to control motion), whereas decreasing Continuity causes sharp transitions while maintaining spacing (enabling sudden motion changes, like bouncing).

Setting Continuity to -1 results in linear interpolation (evenly spaced points along straight lines between the input points).

Setting Tension, Continuity, and Bias all to 0 results in a Catmull–Rom spline.

This particular spline formulation — the Tension/Continuity/Bias parameterization — was developed by Doris H. U. Kochanek and Richard H. Bartels in their 1984 SIGGRAPH paper.
