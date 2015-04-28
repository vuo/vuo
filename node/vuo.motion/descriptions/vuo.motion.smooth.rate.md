Smoothly transitions from one position to another. 

This node always moves toward its target at the same rate, regardless of the distance between the current position and the target position.  (See also the `Smooth with Duration` node.)

Starting when an event hits this node's `setTarget` input port, each time the `time` input port receives an event, the `position` port outputs the smoothed position, until it comes to rest.

The `curve` and `easing` ports select the smoothing curve. This is the easing curve that describes the smoothed position's speed as it travels from the initial position to the target position.

   - `time` — The time at which to calculate the smoothed value.
   - `setPosition` — Sets the initial position, or, when it receives an event, immediately moves to a new position (without any smoothing).
   - `setTarget` — Starts smoothly moving toward a new position.
   - `rate` — How fast the smooth movement should be. This is distance between `setPosition` and `setTarget` covered per unit of time, based on the values coming into the `time` port.
   - `curve` — The shape of the smoothing curve.
   - `easing` — Which part of the smoothing curve is shallow.
