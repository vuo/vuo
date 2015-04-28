Calculates a position along an easing curve. 

This node can be used to animate an object along a curved path. Send gradually changing values to the `time` input port, and connect the `value` output port to the part of the composition that controls the object's position. 

   - `time` — The time at which to calculate the curve.
   - `startPosition` — The position at time 0.
   - `endPosition` — The position when time reaches `duration`.
   - `curve` — The shape of the curve.
   - `easing` — Which part of the curve is shallow.
   - `loop` — How time repeats once it exceeds `duration`.
