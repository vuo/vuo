Calculates a position along an easing curve. 

The output of this node can be used to control the speed, position, or other parameters of an animated object. Send gradually changing values to the `Time` input port, and connect the `Value` output port to the part of the composition that controls the parameter. 

   - `Time` — The time at which to calculate the curve.
   - `Start Position` — The position at the beginning of the curve's cycle (and at the end of the curve's cycle in Mirror Loop mode). When `Time` and `Phase` are both 0, this value is output.
   - `End Position` — The position at the end of the curve's cycle (or halfway through the curve's cycle in Mirror Loop mode). When `Time` = `Duration` and `Phase` = 0, this value is output.
   - `Curve` — The shape of the curve.
   - `Easing` — Which part of the curve is shallow.
   - `Loop` — How time repeats once it exceeds `Duration`.
   - `Phase` — The offset into the curve's cycle at `Time` = 0. A phase of 0.5 means that the curve is shifted by half of a cycle. A phase of 1 means that the curve is shifted by a full cycle, which is the same as a phase of 0. Useful if you have multiple `Curve` nodes and want them to move slightly out of sync.

If `Start Position`, `End Position`, and `Value` are 2D or 3D points (as opposed to real numbers), then each coordinate of `Value` is calculated separately. For example, the X-coordinate of `Value` will fall along a curve based on the X-coordinates of `Start Position` and `End Position`, and the Y-coordinate of `Value` will fall along a curve based on the Y-coordinates of `Start Position` and `End Position`. 
