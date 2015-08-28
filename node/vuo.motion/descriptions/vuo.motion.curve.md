Calculates a position along an easing curve. 

The output of this node can be used to control the speed, position, or other parameters of an animated object. Send gradually changing values to the `time` input port, and connect the `value` output port to the part of the composition that controls the parameter. 

   - `time` — The time at which to calculate the curve.
   - `startPosition` — The position at time 0.
   - `endPosition` — The position when time reaches `duration`.
   - `curve` — The shape of the curve.
   - `easing` — Which part of the curve is shallow.
   - `loop` — How time repeats once it exceeds `duration`.

If `startPosition`, `endPosition`, and `value` are 2D or 3D points (as opposed to real numbers), then each coordinate of `value` is calculated separately. For example, the x-coordinate of `value` will fall along a curve based on the x-coordinates of `startPosition` and `endPosition`, and the y-coordinate of `value` will fall along a curve based on the y-coordinates of `startPosition` and `endPosition`. 
