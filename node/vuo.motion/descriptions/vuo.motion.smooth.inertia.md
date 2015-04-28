Smoothly transitions from one position to another. 

If the target changes before the position has reached the previous target, this node maintains a continuous velocity (unlike the `Smooth with Duration` node).

This node always takes the same amount of time to reach the target, regardless of the distance between the current position and the target position.  (See also the `Smooth with Rate` node.)

Starting when an event hits this node's `setTarget` input port, each time the `time` input port receives an event, the `position` port outputs the smoothed position, until it comes to rest.

   - `time` — The time at which to calculate the smoothed value.
   - `setPosition` — Sets the initial position, or, when it receives an event, immediately moves to a new position (without any smoothing).
   - `setTarget` — Starts smoothly moving toward a new position.
   - `duration` — How long the smooth movement should take, based on the values coming into the `time` port.
