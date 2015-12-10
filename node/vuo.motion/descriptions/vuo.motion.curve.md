Calculates a position along an easing curve. 

The output of this node can be used to control the speed, position, or other parameters of an animated object. Send gradually changing values to the `Time` input port, and connect the `Value` output port to the part of the composition that controls the parameter. 

   - `Time` — The time at which to calculate the curve.
   - `Start Position` — The position at time 0.
   - `End Position` — The position when time reaches `Duration`.
   - `Curve` — The shape of the curve.
   - `Easing` — Which part of the curve is shallow.
   - `Loop` — How time repeats once it exceeds `Duration`.

If `Start Position`, `End Position`, and `Value` are 2D or 3D points (as opposed to real numbers), then each coordinate of `Value` is calculated separately. For example, the X-coordinate of `Value` will fall along a curve based on the X-coordinates of `Start Position` and `End Position`, and the Y-coordinate of `Value` will fall along a curve based on the Y-coordinates of `Start Position` and `End Position`. 
