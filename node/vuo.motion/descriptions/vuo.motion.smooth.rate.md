Smoothly transitions from one position to another. 

This node always moves toward its target at the same rate, regardless of the distance between the current position and the target position.  (See also the `Smooth with Duration` node.)

Starting when an event hits this node's `Set Target` input port, each time the `Time` input port receives an event, the `Position` port outputs the smoothed position, until it comes to rest.

The `Curve` and `Easing` ports select the smoothing curve. This is the easing curve that describes the smoothed position's speed as it travels from the initial position to the target position.

   - `Time` — The time at which to calculate the smoothed value.
   - `Set Position` — Sets the initial position, or, when it receives an event, immediately moves to a new position (without any smoothing).
   - `Set Target` — Starts smoothly moving toward a new position.
   - `Rate` — How fast the smooth movement should be. This is distance between `Set Position` and `Set Target` covered per unit of time, based on the values coming into the `Time` port.
   - `Curve` — The shape of the smoothing curve.
   - `Easing` — Which part of the smoothing curve is shallow.
   - `Reached Target` — Outputs an event when the transition has reached the target and stopped moving.
