Calculates a position along the path of an oscillating spring. 

This node can be used to animate an object in a bouncing or oscillating motion. You can connect the `requestedFrame` output port of a node that displays a graphics window (such as `Render Scene to Window`) to the `time` input port of this node. Starting when an event hits this node's `dropFromPosition` input port, each time the `time` input port receives an event, the `position` port outputs the spring's position, until the spring comes to rest.

   - `time` — A time, such as the output from the `requestedFrame` port of a graphics window node. Each time value should be greater than the previous one.
   - `setRestingPosition` — Sets the spring's resting position — where the spring goes after it has finished oscillating. Changing the resting position causes the position to change immediately (without any springiness) and stops the spring's oscillation.
   - `dropFromPosition` — When this port receives an event, the simulated spring is released at the specified position, and the node begins (or continues) letting `time` events through.
   - `period` — The duration, in seconds, of the spring's oscillations.
   - `damping` — How quickly the spring comes to rest.  When `damping` is 0, the spring oscillates forever.  When `damping` is 1, the spring comes to rest quickly, without oscillating.  When `damping` is between 0 and 1, the spring oscillates and then comes to rest.
