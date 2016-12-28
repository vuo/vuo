Smoothly transitions from one position to another, with a springing motion when it reaches the second position and comes to rest. 

This node can be used to animate an object in a bouncing or oscillating motion. You can connect the `Requested Frame` output port of a node that displays a graphics window (such as `Render Scene to Window`) to the `Time` input port of this node.

Starting when an event hits this node's `Set Target` input port, each time the `Time` input port receives an event, the `Position` port outputs the smoothed position, until the spring comes to rest.

   - `Time` — A time, such as the output from the `Requested Frame` port of a graphics window node. Each time value should be greater than the previous one.
   - `Set Position` — Sets the initial position, or, when it receives an event, immediately moves to a new position (without any smoothing).
   - `Set Target` — Starts smoothly moving toward a new position.
   - `Period` — The duration, in seconds, of the spring's oscillations.
   - `Damping` — How quickly the spring comes to rest.  When `Damping` is 0, the spring oscillates forever.  When `Damping` is 1, the spring comes to rest quickly, without oscillating.  When `Damping` is between 0 and 1, the spring oscillates and then comes to rest.
   - `Reached Target` — Outputs an event when the transition has reached the target and stopped moving.  (Although a spring in real life continues oscillating imperceptibly for a long time, the spring simulated by this node stops moving when the time reaches 2 * Period / Damping.)
