Smoothly transitions from one position to another, with a springing motion when it reaches the second position and comes to rest. 

This node can be used to animate an object in a bouncing or oscillating motion. You can connect the `requestedFrame` output port of a node that displays a graphics window (such as `Render Scene to Window`) to the `time` input port of this node.

Starting when an event hits this node's `setTarget` input port, each time the `time` input port receives an event, the `position` port outputs the smoothed position, until the spring comes to rest.

   - `time` — A time, such as the output from the `requestedFrame` port of a graphics window node. Each time value should be greater than the previous one.
   - `setPosition` — Sets the initial position, or, when it receives an event, immediately moves to a new position (without any smoothing).
   - `setTarget` — Starts smoothly moving toward a new position.
   - `period` — The duration, in seconds, of the spring's oscillations.
   - `damping` — How quickly the spring comes to rest.  When `damping` is 0, the spring oscillates forever.  When `damping` is 1, the spring comes to rest quickly, without oscillating.  When `damping` is between 0 and 1, the spring oscillates and then comes to rest.
